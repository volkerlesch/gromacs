
#ifndef _GMX_RANDOM_H_
#define _GMX_RANDOM_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include <types/simple.h>



/*! \brief Abstract datatype for a random number generator
 *
 * This is a handle to the full state of a random number generator. 
 * You can not access anything inside the gmx_rng structure outside this
 * file.
 */
typedef struct gmx_rng *
gmx_rng_t;


/*! \brief Create a new RNG, seeded from a single integer.
 *
 * If you dont want to pick a seed, just call it as
 * rng=gmx_rng_init(gmx_rng_make_seed()) to seed it from
 * the system time or a random device.
 *
 * \param seed Random seed, unsigned 32-bit integer.
 *
 * \return Reference to a random number generator, or NULL if there was an 
 *         error.
 *
 * \threadsafe Yes.
 */
gmx_rng_t 
gmx_rng_init(unsigned int seed);


/*! \brief Generate a 'random' RNG seed.
 *
 * This routine tries to get a seed from /dev/random if present,
 * and if not it uses time-of-day and process id to generate one.
 *
 * \return 32-bit unsigned integer random seed. 
 *
 * Tip: If you use this in your code, it is a good idea to write the
 * returned random seed to a logfile, so you can recreate the exact sequence
 * of random number if you need to reproduce your run later for one reason
 * or another.
 *
 * \threadsafe Yes.
 */
unsigned int
gmx_rng_make_seed(void);


/*! \brief Initialize a RNG with 624 integers (>32 bits of entropy).
 *
 *  The Mersenne twister RNG used in Gromacs has an extremely long period,
 *  but when you only initialize it with a 32-bit integer there are only
 *  2^32 different possible sequences of number - much less than the generator
 *  is capable of.
 *
 *  If you really need the full entropy, this routine makes it possible to
 *  initialize the RNG with up to 624 32-bit integers, which will give you 
 *  up to 2^19968 bits of entropy.
 *
 *  \param seed Array of unsigned integers to form a seed
 *  \param seed_length Number of integers in the array, up to 624 are used.
 *
 * \return Reference to a random number generator, or NULL if there was an 
 *         error.
 *
 * \threadsafe Yes.
 */
gmx_rng_t 
gmx_rng_init_array(unsigned int    seed[], 
		   int             seed_length);


/*! \brief Release resources of a RNG
 *
 *  This routine destroys a random number generator and releases all
 *  resources allocated by it.
 *
 *  \param rng Handle to random number generator previously returned by
 *		       gmx_rng_init() or gmx_rng_init_array().
 *
 * \threadsafe Function itself is threadsafe, but you should only destroy a 
 *             certain RNG once (i.e. from one thread).
 */
void
gmx_rng_destroy(gmx_rng_t rng);


/*! \brief Random 32-bit integer from a uniform distribution
 *
 *  This routine returns a random integer from the random number generator
 *  provided, and updates the state of that RNG.
 *
 *  \param rng Handle to random number generator previously returned by
 *		       gmx_rng_init() or gmx_rng_init_array().
 *
 *  \return 32-bit unsigned integer from a uniform distribution.
 *
 *  \threadsafe Function yes, input data no. You should not call this function 
 *	        from two different threads using the same RNG handle at the
 *              same time. For performance reasons we cannot lock the handle 
 *              with a mutex every time we need a random number - that would 
 *              slow the routine down a factor 2-5. There are two simple 
 *		solutions: either use a mutex and lock it before calling
 *              the function, or use a separate RNG handle for each thread.
 */
unsigned int 
gmx_rng_uniform_uint32(gmx_rng_t rng);


/*! \brief Random gmx_real_t 0<=x<1 from a uniform distribution
 *
 *  This routine returns a random floating-point number from the 
 *  random number generator provided, and updates the state of that RNG.
 *
 *  \param rng Handle to random number generator previously returned by
 *		       gmx_rng_init() or gmx_rng_init_array().
 *
 *  \return floating-point number 0<=x<1 from a uniform distribution.
 *
 *  \threadsafe Function yes, input data no. You should not call this function 
 *		from two different threads using the same RNG handle at the
 *              same time. For performance reasons we cannot lock the handle 
 *              with a mutex every time we need a random number - that would 
 *              slow the routine down a factor 2-5. There are two simple 
 *		solutions: either use a mutex and lock it before calling
 *              the function, or use a separate RNG handle for each thread.
 */
real
gmx_rng_uniform_real(gmx_rng_t rng);


/*! \brief Random gmx_real_t from a gaussian distribution
 *
 *  This routine returns a random floating-point number from the 
 *  random number generator provided, and updates the state of that RNG.
 *  
 *  The Box-Muller algorithm is used to provide gaussian random numbers. This
 *  is not the fastest known algorithm for gaussian numbers, but in contrast
 *  to the alternatives it is very well studied and you can trust the returned
 *  random numbers to have good properties and no correlations.
 *
 *  \param rng Handle to random number generator previously returned by
 *			  gmx_rng_init() or gmx_rng_init_array().
 *
 *  \return Gaussian random floating-point number with average 0.0 and 
 *	    standard deviation 1.0. You can get any average/mean you want
 *          by first multiplying with the desired average and then adding
 *          the average you want.
 *
 *  \threadsafe Function yes, input data no. You should not call this function 
 *		from two different threads using the same RNG handle at the
 *              same time. For performance reasons we cannot lock the handle 
 *              with a mutex every time we need a random number - that would 
 *              slow the routine down a factor 2-5. There are two simple 
 *		solutions: either use a mutex and lock it before calling
 *              the function, or use a separate RNG handle for each thread.
 *
 *  It works perfectly to mix calls to get uniform and gaussian random numbers
 *  from the same generator, but since it will affect the sequence of returned
 *  numbers it is probably better to use separate random number generator
 *  structures.
 */
real
gmx_rng_gaussian_real(gmx_rng_t rng);



/* Return a new gaussian random number with expectation value
 * 0.0 and standard deviation 1.0. This routine uses a table
 * lookup for maximum speed.
 *
 * WARNING: The lookup table is 16k by default, which means
 *          the granularity of the random numbers is coarser
 *	    than what you get from gmx_rng_gauss_real().
 *          In most cases this is no problem whatsoever,
 *          and it is particularly true for BD/SD integration.
 *	    Note that you will NEVER get any really extreme 
 *          numbers: the maximum absolute value returned is
 *          4.0255485.
 *
 * threadsafe: yes
 */
real
gmx_rng_gaussian_table(gmx_rng_t rng);


#endif /* _GMX_RANDOM_H_ */

