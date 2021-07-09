/** @file fbmath.h
 *
 * Header for the different math operations in Fuzzball.  This is mostly
 * used in support of MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef FBMATH_H
#define FBMATH_H

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if defined(HUGE_VAL)
# define INF (HUGE_VAL)     /**< Infinity */
# define NINF (-HUGE_VAL)   /**< Negative infinity */
#else
# define INF (9.9E999)      /**< Infinity */
# define NINF (-9.9E999)    /**< Negative infinity */
#endif

#ifndef NAN
# ifdef  __GNUC__
#  define NAN \
  (__extension__                                                            \
   ((union { unsigned __l __attribute__((__mode__(__SI__))); float __d; })  \
    { __l: 0x7fc00000UL }).__d)
# else
#  ifdef WIN_VC
#   define __nan_bytes           { 0, 0, 0xc0, 0x7f } /**< Not a number def */
#  else
#   include <endian.h>
#   if __BYTE_ORDER == __BIG_ENDIAN
#    define __nan_bytes           { 0x7f, 0xc0, 0, 0 } /**< NaN def */
#   endif
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#    define __nan_bytes           { 0, 0, 0xc0, 0x7f } /**< Not a number def */
#   endif
#  endif
static union { unsigned char __c[4]; float __d; } __nan_union = { __nan_bytes };
#  define NAN    (__nan_union.__d) /**< Constant to use for not a number */
# endif
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846    /**< Pi */
#endif

#ifndef M_PI_2
# define M_PI_2 1.57079632679489661923  /**< Pi/2 */
#endif

/**
 * Determine the minimum value of p vs. q
 *
 * @param p the first value to check
 * @param q the second value to check
 * @return the smaller of the two values
 */
#define MIN(p,q) ((p >= q) ? q : p)

/**
 * Determine the maximum value of p vs. q
 *
 * @param p the first value to check
 * @param q the second value to check
 * @return the larger of the two values
 */
#define MAX(p,q) ((p >= q) ? p : q)

/**
 * Generate a random floating point number
 *
 * The number will be between 0 and 1
 *
 * @return random floating point number
 */
double _int_f_rand(void);

/**
 * Check to see if 'test' is within valid bounds
 *
 * If this returns false, then floating point i_bounds error flag should
 * be set.
 *
 * @param test the value to test
 * @return boolean true if test is within bounds, false otherwise
 */
int arith_good(double test);

/**
 * Make sure basic mathematical operations are valid between types.
 *
 * The types should be the 'type' field of struct inst
 *
 * @param op1_type the first type operand
 * @param op2_type the second type operand
 * @return boolean true if operations are possible, false if not
 */
int arith_type(short op1_type, short op2_type);

/**
 * Are comparative operations allowed for the given struct inst type?
 *
 * Returns true if comparison operations such as greater than, less than,
 * etc. are allowed for a given type.
 *
 * @param op_type the struct inst type field to check
 * @return boolean true if comparison operations are allowed, false otherwise
 */
int comp_t(short op_type);

/**
 * Initialize a random number seed buffer
 *
 * Each struct frame has its own random number seed, which is a buffer
 * of 4 uint32_t's
 *
 * If seed is NULL, we will use the system clock to generate our seed.
 * Otherwise, 16 bytes worth of 'seed' will be copied into our seed buffer.
 *
 * Thus, seed must be at least 16 bytes long if provided.
 *
 * Memory is allocated by this function; the caller is responsible for
 * freeing it at some point.
 *
 * @param seed the seed string which is either 16+ bytes or NULL
 * @return a newly allocated seed buffer
 */
void *init_seed(char *seed);

/**
 * Generate an MD5 Base64 string
 *
 * dest buffer MUST be at least 24 chars long.
 *
 * @param dest the destination buffer - at least 24 characters in length
 * @param orig the original value to generate an MD5 sum for
 * @param len the length of orig
 */
void MD5base64(char *dest, const void *orig, size_t len);

/**
 * Generate an MD5 as a hex string
 *
 * If my math is correct, the 'dest' buffer must be at least 31 characters
 * in size.  Looks like where this is used in the codebase, the buffer is 33
 * characters in size, so that is probably the safer number.
 *
 * @param dest the destination buffer
 * @param orig the original data to MD5
 * @param len the length of orig
 */
void MD5hex(void *dest, const void *orig, size_t len);

/**
 * Check to see if 'test' is a valid double and not INF or NAN.
 *
 * @param test the double to test
 * @return boolean true if good, false if not
 */
int no_good(double test);

/**
 * Do a seeded random number generation
 *
 * This is done by taking the given buffer (which would usually be the
 * frame's rndbuf) and doing an MD5 hash on it, returning the first
 * integer value of the resultant hash.
 *
 * The computed hash is stored in 'buffer', so that a subsequent call
 * hashes the hash, thus ensuring some degree of randomness.
 *
 * @param buffer the seed buffer
 * @return the random integer value
 */
uint32_t rnd(void *buffer);

#endif /* !FBMATH_H */
