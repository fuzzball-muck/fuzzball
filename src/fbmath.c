/** @file fbmath.c
 *
 * Source for the different math operations in Fuzzball.  This is mostly
 * used in support of MUF.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"

#include "fbmath.h"
#include "inst.h"

/**
 * Generate a random floating point number
 *
 * The number will be between 0 and 1
 *
 * @return random floating point number
 */
double
_int_f_rand(void)
{
    return ((double)rand() / (double)RAND_MAX);
}

/**
 * Check to see if 'test' is within valid bounds
 *
 * If this returns false, then floating point i_bounds error flag should
 * be set.
 *
 * @param the value to test
 * @return boolean true if test is within bounds, false otherwise
 */
int
arith_good(double test)
{
    return ((test <= (double) (INT_MAX)) && (test >= (double) (INT_MIN)));
}

/**
 * Make sure basic mathematical operations are valid between types.
 *
 * The types should be the 'type' field of struct inst, and is for checking
 * if operations like addition, subtraction, etc. are possible.
 *
 * @param op1_type the first type operand
 * @param op2_type the second type operand
 * @return boolean true if operations are possible, false if not
 */
int
arith_type(short op1_type, short op2_type)
{
    return ((op1_type == PROG_INTEGER && op2_type == PROG_INTEGER)
            || (op1_type == PROG_OBJECT && op2_type == PROG_INTEGER)
            || (op1_type == PROG_VAR && op2_type == PROG_INTEGER)
            || (op1_type == PROG_LVAR && op2_type == PROG_INTEGER)
            || (op1_type == PROG_FLOAT && op2_type == PROG_FLOAT)
            || (op1_type == PROG_FLOAT && op2_type == PROG_INTEGER)
            || (op1_type == PROG_INTEGER && op2_type == PROG_FLOAT));
}

/**
 * Are comparative operations allowed for the given struct inst type?
 *
 * Returns true if comparison operations such as greater than, less than,
 * etc. are allowed for a given type.
 *
 * @param op_type the struct inst type field to check
 * @return boolean true if comparison operations are allowed, false otherwise
 */
int
comp_t(short op_type)
{
    return (op_type == PROG_INTEGER || op_type == PROG_FLOAT 
            || op_type == PROG_OBJECT);
}

/**
 * Check to see if 'test' is a valid double and not INF or NINF.
 *
 * @param test the double to test
 * @return boolean true if good, false if not
 */
int
no_good(double test)
{
    return test == INF || test == NINF;
}

/**************************************************************************
 *
 * MD5 AND BASE64 IMPLEMENTATIONS
 *
 * Please note that a minimal effort was put into documenting these
 * functions; they were probably copied originally from reference code
 * bases and are very common non-fuzzball-specific implementations.
 *
 **************************************************************************/

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
               (w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

/* Context for keeping track of an MD5 calculation */
struct xMD5Context {
    uint32_t buf[4];
    uint32_t bytes[2];
    uint32_t in[16];
};

/**
 * Do an MD5 Transformation
 *
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 *
 * @private
 * @param buf transformation buffer
 * @param in input values
 */
static void
xMD5Transform(uint32_t buf[4], uint32_t const in[16])
{
    register uint32_t a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5STEP(F1, a, b, c, d, in[0] + 0xd76aa478, 7);
    MD5STEP(F1, d, a, b, c, in[1] + 0xe8c7b756, 12);
    MD5STEP(F1, c, d, a, b, in[2] + 0x242070db, 17);
    MD5STEP(F1, b, c, d, a, in[3] + 0xc1bdceee, 22);
    MD5STEP(F1, a, b, c, d, in[4] + 0xf57c0faf, 7);
    MD5STEP(F1, d, a, b, c, in[5] + 0x4787c62a, 12);
    MD5STEP(F1, c, d, a, b, in[6] + 0xa8304613, 17);
    MD5STEP(F1, b, c, d, a, in[7] + 0xfd469501, 22);
    MD5STEP(F1, a, b, c, d, in[8] + 0x698098d8, 7);
    MD5STEP(F1, d, a, b, c, in[9] + 0x8b44f7af, 12);
    MD5STEP(F1, c, d, a, b, in[10] + 0xffff5bb1, 17);
    MD5STEP(F1, b, c, d, a, in[11] + 0x895cd7be, 22);
    MD5STEP(F1, a, b, c, d, in[12] + 0x6b901122, 7);
    MD5STEP(F1, d, a, b, c, in[13] + 0xfd987193, 12);
    MD5STEP(F1, c, d, a, b, in[14] + 0xa679438e, 17);
    MD5STEP(F1, b, c, d, a, in[15] + 0x49b40821, 22);

    MD5STEP(F2, a, b, c, d, in[1] + 0xf61e2562, 5);
    MD5STEP(F2, d, a, b, c, in[6] + 0xc040b340, 9);
    MD5STEP(F2, c, d, a, b, in[11] + 0x265e5a51, 14);
    MD5STEP(F2, b, c, d, a, in[0] + 0xe9b6c7aa, 20);
    MD5STEP(F2, a, b, c, d, in[5] + 0xd62f105d, 5);
    MD5STEP(F2, d, a, b, c, in[10] + 0x02441453, 9);
    MD5STEP(F2, c, d, a, b, in[15] + 0xd8a1e681, 14);
    MD5STEP(F2, b, c, d, a, in[4] + 0xe7d3fbc8, 20);
    MD5STEP(F2, a, b, c, d, in[9] + 0x21e1cde6, 5);
    MD5STEP(F2, d, a, b, c, in[14] + 0xc33707d6, 9);
    MD5STEP(F2, c, d, a, b, in[3] + 0xf4d50d87, 14);
    MD5STEP(F2, b, c, d, a, in[8] + 0x455a14ed, 20);
    MD5STEP(F2, a, b, c, d, in[13] + 0xa9e3e905, 5);
    MD5STEP(F2, d, a, b, c, in[2] + 0xfcefa3f8, 9);
    MD5STEP(F2, c, d, a, b, in[7] + 0x676f02d9, 14);
    MD5STEP(F2, b, c, d, a, in[12] + 0x8d2a4c8a, 20);

    MD5STEP(F3, a, b, c, d, in[5] + 0xfffa3942, 4);
    MD5STEP(F3, d, a, b, c, in[8] + 0x8771f681, 11);
    MD5STEP(F3, c, d, a, b, in[11] + 0x6d9d6122, 16);
    MD5STEP(F3, b, c, d, a, in[14] + 0xfde5380c, 23);
    MD5STEP(F3, a, b, c, d, in[1] + 0xa4beea44, 4);
    MD5STEP(F3, d, a, b, c, in[4] + 0x4bdecfa9, 11);
    MD5STEP(F3, c, d, a, b, in[7] + 0xf6bb4b60, 16);
    MD5STEP(F3, b, c, d, a, in[10] + 0xbebfbc70, 23);
    MD5STEP(F3, a, b, c, d, in[13] + 0x289b7ec6, 4);
    MD5STEP(F3, d, a, b, c, in[0] + 0xeaa127fa, 11);
    MD5STEP(F3, c, d, a, b, in[3] + 0xd4ef3085, 16);
    MD5STEP(F3, b, c, d, a, in[6] + 0x04881d05, 23);
    MD5STEP(F3, a, b, c, d, in[9] + 0xd9d4d039, 4);
    MD5STEP(F3, d, a, b, c, in[12] + 0xe6db99e5, 11);
    MD5STEP(F3, c, d, a, b, in[15] + 0x1fa27cf8, 16);
    MD5STEP(F3, b, c, d, a, in[2] + 0xc4ac5665, 23);

    MD5STEP(F4, a, b, c, d, in[0] + 0xf4292244, 6);
    MD5STEP(F4, d, a, b, c, in[7] + 0x432aff97, 10);
    MD5STEP(F4, c, d, a, b, in[14] + 0xab9423a7, 15);
    MD5STEP(F4, b, c, d, a, in[5] + 0xfc93a039, 21);
    MD5STEP(F4, a, b, c, d, in[12] + 0x655b59c3, 6);
    MD5STEP(F4, d, a, b, c, in[3] + 0x8f0ccc92, 10);
    MD5STEP(F4, c, d, a, b, in[10] + 0xffeff47d, 15);
    MD5STEP(F4, b, c, d, a, in[1] + 0x85845dd1, 21);
    MD5STEP(F4, a, b, c, d, in[8] + 0x6fa87e4f, 6);
    MD5STEP(F4, d, a, b, c, in[15] + 0xfe2ce6e0, 10);
    MD5STEP(F4, c, d, a, b, in[6] + 0xa3014314, 15);
    MD5STEP(F4, b, c, d, a, in[13] + 0x4e0811a1, 21);
    MD5STEP(F4, a, b, c, d, in[4] + 0xf7537e82, 6);
    MD5STEP(F4, d, a, b, c, in[11] + 0xbd3af235, 10);
    MD5STEP(F4, c, d, a, b, in[2] + 0x2ad7d2bb, 15);
    MD5STEP(F4, b, c, d, a, in[9] + 0xeb86d391, 21);

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

/**
 * MD5 Byte Swap for endian-ness
 *
 * Shuffle the bytes into little-endian order within words, as per the
 * MD5 spec.  Note: this code works regardless of the byte order.
 *
 * @private
 * @param buf the buffer to transform
 * @param words the number of words to transform
 */
static void
byteSwap(uint32_t * buf, unsigned words)
{
    uint8_t *p = (uint8_t *) buf;

    do {
        *buf++ = (uint32_t) ((unsigned) p[3] << 8 | p[2]) << 16 | ((unsigned) p[1] << 8 | p[0]);
        p += 4;
    } while (--words);
}

/**
 * Initialize MD5 Context
 *
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 *
 * @private
 * @param ctx the context to initialize
 */
static void
xMD5Init(struct xMD5Context *ctx)
{
    ctx->buf[0] = 0x67452301;
    ctx->buf[1] = 0xefcdab89;
    ctx->buf[2] = 0x98badcfe;
    ctx->buf[3] = 0x10325476;

    ctx->bytes[0] = 0;
    ctx->bytes[1] = 0;
}

/**
 * Update context to reflect the concatenation of another buffer full of bytes.
 *
 * @private
 * @param ctx the context to work on
 * @param buf the input buffer
 * @param len the size of the input buffer
 */
static void
xMD5Update(struct xMD5Context *ctx, const uint8_t * buf, size_t len)
{
    uint32_t t;

    /* Update byte count */

    t = ctx->bytes[0];

    if ((ctx->bytes[0] = t + len) < t)
        ctx->bytes[1]++;    /* Carry from low to high */

    t = 64 - (t & 0x3f);    /* Space available in ctx->in (at least 1) */

    if ((unsigned) t > (unsigned) len) {
        memmove((uint8_t *) ctx->in + 64 - (unsigned) t, buf, len);
        return;
    }

    /* First chunk is an odd size */
    memmove((uint8_t *) ctx->in + 64 - (unsigned) t, buf, (unsigned) t);
    byteSwap(ctx->in, 16);
    xMD5Transform(ctx->buf, ctx->in);
    buf += (unsigned) t;
    len -= (unsigned) t;

    /* Process data in 64-byte chunks */
    while (len >= 64) {
        memmove((uint8_t *) ctx->in, buf, 64);
        byteSwap(ctx->in, 16);
        xMD5Transform(ctx->buf, ctx->in);
        buf += 64;
        len -= 64;
    }

    /* Handle any remaining bytes of data. */
    memmove((uint8_t *) ctx->in, buf, len);
}

/**
 * Finalize an MD5 digest
 *
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 *
 * @private
 * @param digest the resulting MD5 digest
 * @param ctx the MD5 context
 */
static void
xMD5Final(uint8_t digest[16], struct xMD5Context *ctx)
{
    int count = (int) (ctx->bytes[0] & 0x3f);   /* Bytes in ctx->in */
    uint8_t *p = (uint8_t *) ctx->in + count;   /* First unused byte */

    /* Set the first char of padding to 0x80.  There is always room. */
    *p++ = 0x80;

    /* Bytes of padding needed to make 56 bytes (-8..55) */
    count = 56 - 1 - count;

    if (count < 0) {    /* Padding forces an extra block */
        memset(p, 0, count + 8);
        byteSwap(ctx->in, 16);
        xMD5Transform(ctx->buf, ctx->in);
        p = (uint8_t *) ctx->in;
        count = 56;
    }

    memset(p, 0, count + 8);
    byteSwap(ctx->in, 14);

    /* Append length in bits and transform */
    ctx->in[14] = ctx->bytes[0] << 3;
    ctx->in[15] = ctx->bytes[1] << 3 | ctx->bytes[0] >> 29;
    xMD5Transform(ctx->buf, ctx->in);

    byteSwap(ctx->buf, 4);
    memmove(digest, (uint8_t *) ctx->buf, 16);
    memset((uint8_t *) ctx, 0, sizeof(ctx));
}

/**
 * Perform an MD5 hash
 *
 * dest buffer MUST be at least 16 bytes long.
 *
 * @private
 * @param dest the destination buffer - must be at least 16 bytes
 * @param orig the original value to hash
 * @param len the length of the original value
 */
static void
MD5hash(void *dest, const void *orig, size_t len)
{
    struct xMD5Context context;

    xMD5Init(&context);
    xMD5Update(&context, (const uint8_t *) orig, len);
    xMD5Final((uint8_t *) dest, &context);
}

/**
 * Do a base 64 encoding
 *
 * outbuf MUST be at least (((inlen+2)/3)*4)+1 chars long.
 *
 * More simply, make sure your output buffer is at least 4/3rds the size
 * of the input buffer, plus five bytes.
 *
 * @private
 * @param outbuf the output buffer which must be sized as noted above
 * @param inbuf the input buffer
 * @param inlen the length of the input buffer
 */
static void
Base64Encode(char *outbuf, const void *inbuf, size_t inlen)
{
    const unsigned char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const unsigned char *inb = (unsigned char *) inbuf;
    unsigned char *out = NULL;
    size_t numb;
    size_t endcnt;
    size_t i;

    numb = inlen;

    if (numb > 0) {
        unsigned int acc = 0;
        out = (unsigned char *) outbuf;

        for (i = 0; i < numb; i++) {
            if (i % 3 == 0) {
                acc = inb[i];
            } else if (i % 3 == 1) {
                acc <<= 8;
                acc |= inb[i];
            } else {
                acc <<= 8;
                acc |= inb[i];

                *out++ = b64[(acc >> 18) & 0x3f];
                *out++ = b64[(acc >> 12) & 0x3f];
                *out++ = b64[(acc >> 6) & 0x3f];
                *out++ = b64[acc & 0x3f];
            }
        }

        if (i % 3 == 0) {
            endcnt = 0;
        } else if (i % 3 == 1) {
            endcnt = 2;
        } else {
            endcnt = 1;
        }

        for (; i % 3; i++) {
            acc <<= 8;
        }

        if (endcnt > 0) {
            *out++ = b64[(acc >> 18) & 0x3f];
            *out++ = b64[(acc >> 12) & 0x3f];

            if (endcnt < 2)
                *out++ = b64[(acc >> 6) & 0x3f];

            if (endcnt < 1)
                *out++ = b64[acc & 0x3f];

            while (endcnt-- > 0)
                *out++ = '=';
        }
    }

    *out++ = '\0';

    out = (unsigned char *) outbuf;

    while (*out) {
        if (*out++ > 127)
            abort();
    }
}

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
void
MD5hex(void *dest, const void *orig, size_t len)
{
    unsigned char tmp[16];

    MD5hash(tmp, orig, len);

    for (int i = 0; i < 16; i++) {
        snprintf((char *)dest + (i*2), 255, "%.2x", tmp[i]);
    }
}

/**
 * Generate an MD5 Base64 string
 *
 * dest buffer MUST be at least 24 chars long.
 *
 * @param dest the destination buffer - at least 24 characters in length
 * @param orig the original value to generate an MD5 sum for
 * @param len the length of orig
 */
void
MD5base64(char *dest, const void *orig, size_t len)
{
    void *tmp = malloc(16);
    MD5hash(tmp, orig, len);
    Base64Encode(dest, tmp, 16);
    free(tmp);
}

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
void *
init_seed(char *seed)
{
    uint32_t *digest;
    int tbuf[8];

    if (!(digest = malloc(sizeof(uint32_t) * 4))) {
        return (NULL);
    }

    if (!seed) {
        /* No fixed seed given... make something up */
        srand((unsigned int) time(NULL));

        for (int loop = 0; loop < 8; loop++)
            tbuf[loop] = rand();

        memcpy(digest, tbuf, 16);
    } else {
        memcpy(digest, seed, 16);
    }

    return ((void *) digest);
}

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
uint32_t
rnd(void *buffer)
{
    uint32_t *digest = (uint32_t *) buffer;

    if (!digest)
        return (0);

    MD5hash(digest, digest, sizeof(digest));
    return (digest[0]);
}
