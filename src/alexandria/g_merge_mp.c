/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; c-file-style: "stroustrup"; -*-
 * $Id: merge_mp.c,v 1.1 2009/05/17 13:57:48 spoel Exp $
 * 
 *                This source code is part of
 * 
 *                 G   R   O   M   A   C   S
 * 
 *          GROningen MAchine for Chemical Simulations
 * 
 *                        VERSION 4.0.99
 * Written by David van der Spoel, Erik Lindahl, Berk Hess, and others.
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2008, The GROMACS development team,
 * check out http://www.gromacs.org for more information.

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * If you want to redistribute modifications, please consider that
 * scientific software is very special. Version control is crucial -
 * bugs must be traceable. We will be happy to consider code for
 * inclusion in the official distribution, but derived work must not
 * be called official GROMACS. Details are found in the README & COPYING
 * files - if they are missing, get the official version at www.gromacs.org.
 * 
 * To help us fund GROMACS development, we humbly ask that you cite
 * the papers on the package - you can find them in the top README file.
 * 
 * For more info, check our website at http://www.gromacs.org
 * 
 * And Hey:
 * Groningen Machine for Chemical Simulation
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "maths.h"
#include "futil.h"
#include "smalloc.h"
#include "string2.h"
#include "vec.h"
#include "statutil.h"
#include "copyrite.h"
#include "gstat.h"
#include "gmx_fatal.h"
#include "molprop.h"
#include "molprop_util.h"
#include "poldata.h"
#include "poldata_xml.h"
#include "molprop_xml.h"
#include "molprop_sqlite3.h"

typedef struct {
    char *iupac;
    char *prop;
    char *value;
    char *ref;
} t_prop;

static int tp_comp(const void *a,const void *b)
{
    t_prop *ta = (t_prop *)a;
    t_prop *tb = (t_prop *)b;
    
    return strcasecmp(ta->iupac,tb->iupac);
}

static void add_properties(const char *fn,int nmp,gmx_molprop_t mp[])
{
    FILE   *fp;
    int    nprop=0;
    t_prop *tp=NULL,key,*tpp;
    char   buf[STRLEN];
    char   **ptr;
    int    i,expref,nadd=0;
    
    if (NULL != fn) {
        fp = ffopen(fn,"r");
        while (!feof(fp)) {
            fgets2(buf,STRLEN-1,fp);
            ptr = split('|',buf);
            if ((NULL != ptr) &&
                (NULL != ptr[0]) && (NULL != ptr[1]) &&
                (NULL != ptr[2]) && (NULL != ptr[3])) {
                srenew(tp,++nprop);
                tp[nprop-1].iupac = strdup(ptr[0]);
                tp[nprop-1].prop = strdup(ptr[1]);
                tp[nprop-1].value = strdup(ptr[2]);
                tp[nprop-1].ref = strdup(ptr[3]);
                sfree(ptr[0]);
                sfree(ptr[1]);
                sfree(ptr[2]);
                sfree(ptr[3]);
                sfree(ptr);
            }
        }
        printf("Read in %d properties from %s.\n",nprop,fn);
        qsort(tp,nprop,sizeof(tp[0]),tp_comp);
        fclose(fp);
        for(i=0; (i<nmp); i++) {
            key.iupac = gmx_molprop_get_iupac(mp[i]);
            if (NULL != key.iupac) {
                tpp = bsearch(&key,tp,nprop,sizeof(tp[0]),tp_comp);
                if (NULL != tpp) {
                    gmx_molprop_add_experiment(mp[i],tpp->ref,"minimum",&expref);
                    gmx_molprop_add_energy(mp[i],expref,tpp->prop,"kJ/mol",
                                           atof(tpp->value),0);
                    nadd++;
                }
            }
        }
        printf("Added properties for %d out of %d molecules.\n",nadd,nmp);
    }
}

int main(int argc,char *argv[])
{
    static const char *desc[] = 
    {
      "merge_mp reads multiple molprop files and merges the molecule descriptions",
      "into a single new file. By specifying the [TT]-db[TT] option additional experimental",
      "information will be read from a SQLite3 database.[PAR]",
    };
    t_filenm fnm[] = 
    {
        { efDAT, "-f",  "data",      ffRDMULT },
        { efDAT, "-o",  "allmols",   ffWRITE },
        { efDAT, "-di", "gentop",    ffOPTRD },
        { efDAT, "-db", "sqlite",    ffOPTRD },
        { efDAT, "-x",  "extra",     ffOPTRD }
    };
    int NFILE = (sizeof(fnm)/sizeof(fnm[0]));
    static char *sort[] = { NULL, "molname", "formula", "composition", NULL };
    static int compress=1;
    static real th_toler=170,ph_toler=5;
    t_pargs pa[] = 
    {
        { "-sort",   FALSE, etENUM, {sort},
          "Key to sort the final data file on." },
        { "-th_toler",FALSE, etREAL, {&th_toler},
          "If bond angles are larger than this value the group will be treated as a linear one and a virtual site will be created to keep the group linear" },
        { "-ph_toler", FALSE, etREAL, {&ph_toler},
          "If dihedral angles are less than this (in absolute value) the atoms will be treated as a planar group with an improper dihedral being added to keep the group planar" },
        { "-compress", FALSE, etBOOL, {&compress},
          "Compress output XML files" }
    };
    FILE   *fp,*gp;
    char   **fns;
    int    i,alg,np,eMP,nfiles;
    int    cur = 0;
#define prev (1-cur)
    gmx_molprop_t *mp=NULL;
    gmx_atomprop_t ap;
    gmx_poldata_t  pd;
    output_env_t   oenv;
    
    CopyRight(stdout,argv[0]);
    
    parse_common_args(&argc,argv,PCA_NOEXIT_ON_ARGS,NFILE,fnm,
                      sizeof(pa)/sizeof(pa[0]),pa,
                      sizeof(desc)/sizeof(desc[0]),desc,
                      0,NULL,&oenv);
		    
    ap = gmx_atomprop_init();
    if ((pd = gmx_poldata_read(opt2fn_null("-di",NFILE,fnm),ap)) == NULL)
      gmx_fatal(FARGS,"Can not read the force field information. File missing or incorrect.");
    nfiles = opt2fns(&fns,"-f",NFILE,fnm);
    mp = merge_xml(nfiles,fns,NULL,NULL,NULL,&np,ap,pd,TRUE,TRUE,th_toler,ph_toler);
    
    add_properties(opt2fn_null("-x",NFILE,fnm),np,mp);

    gmx_molprop_read_sqlite3(np,mp,opt2fn_null("-db",NFILE,fnm));
        
    gmx_molprops_write(opt2fn("-o",NFILE,fnm),np,mp,compress);
  
    thanx(stdout);
  
    return 0;
}
