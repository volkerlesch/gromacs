/*
 * $Id$
 * 
 *       This source code is part of
 * 
 *        G   R   O   M   A   C   S
 * 
 * GROningen MAchine for Chemical Simulations
 * 
 *               VERSION 2.0
 * 
 * Copyright (c) 1991-1999
 * BIOSON Research Institute, Dept. of Biophysical Chemistry
 * University of Groningen, The Netherlands
 * 
 * Please refer to:
 * GROMACS: A message-passing parallel molecular dynamics implementation
 * H.J.C. Berendsen, D. van der Spoel and R. van Drunen
 * Comp. Phys. Comm. 91, 43-56 (1995)
 * 
 * Also check out our WWW page:
 * http://md.chem.rug.nl/~gmx
 * or e-mail to:
 * gromacs@chem.rug.nl
 * 
 * And Hey:
 * Great Red Oystrich Makes All Chemists Sane
 */

#ifndef _pp2shift_h
#define _pp2shift_h

static char *SRCID_pp2shift_h = "$Id$";

#include "typedefs.h"
	
/* must correspond with 'leg' g_chi.c:727 */
enum { edPhi=0, edPsi, edOmega, edChi1, edChi2, edChi3, edChi4, edChi5, edChi6, edMax };

#define NHISTO 360
#define NONCHI 3
#define MAXCHI edMax-NONCHI

typedef struct {
  int minO,minC,H,N,C,O,Cn[MAXCHI+3];
} t_dihatms; /* Cn[0]=N, Cn[1]=Ca, Cn[2]=Cb etc. */

typedef struct {
  char name[12];
  int  resnr;
  int  index;       /* Index for amino acids (histograms) */
  int  j0[edMax];   /* Index in dih array (phi angle is first...) */
  t_dihatms  atm;
  int  b[edMax];
  int  ntr[edMax];
  real S2[edMax];
} t_dlist;

extern void do_pp2shifts(FILE *fp,int nframes,
			 int nlist,t_dlist dlist[],real **dih);

extern bool has_dihedral(int Dih,t_dlist *dl);

extern t_dlist *mk_dlist(FILE *log, 
			 t_atoms *atoms, int *nlist,
			 bool bPhi, bool bPsi, bool bChi, int maxchi,
			 int r0,int naa,char **aa);
			 
extern void pr_dlist(FILE *fp,int nl,t_dlist dl[],real dt);

extern int pr_trans(FILE *fp,int nl,t_dlist dl[],real dt,int Xi);


#endif
