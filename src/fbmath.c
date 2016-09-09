#include "config.h"

#include "fbmath.h"
#include "inst.h"

float
_int_f_rand(void)
{
    return (rand() / (float) RAND_MAX);
}

int
arith_good(double test)
{
    return ((test <= (double) (MAXINT)) && (test >= (double) (MININT)));
}

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

int
comp_t(short op_type)
{
    return (op_type == PROG_INTEGER || op_type == PROG_FLOAT || op_type == PROG_OBJECT);
}

int
no_good(double test)
{
    return test == INF || test == NINF;
}

/* The four core functions - F1 is optimized somewhat */

/* #define F1(x, y, z) (x & y | ~x & z) */
#define F1(x, y, z) (z ^ (x & (y ^ z)))
#define F2(x, y, z) F1(z, x, y)
#define F3(x, y, z) (x ^ y ^ z)
#define F4(x, y, z) (y ^ (x | ~z))

/* This is the central step in the MD5 algorithm. */
#define MD5STEP(f,w,x,y,z,in,s) \
	 (w += f(x,y,z) + in, w = (w<<s | w>>(32-s)) + x)

struct xMD5Context {
    word32 buf[4];
    word32 bytes[2];
    word32 in[16];
};

/*
 * The core of the MD5 algorithm, this alters an existing MD5 hash to
 * reflect the addition of 16 longwords of new data.  MD5Update blocks
 * the data and converts bytes into longwords for this routine.
 */
static void
xMD5Transform(word32 buf[4], word32 const in[16])
{
    register word32 a, b, c, d;

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

/*
 * Shuffle the bytes into little-endian order within words, as per the
 * MD5 spec.  Note: this code works regardless of the byte order.
 */
static void
byteSwap(word32 * buf, unsigned words)
{
    byte *p = (byte *) buf;

    do {
	*buf++ = (word32) ((unsigned) p[3] << 8 | p[2]) << 16 | ((unsigned) p[1] << 8 | p[0]);
	p += 4;
    } while (--words);
}

/*
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
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

/*
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 */
static void
xMD5Update(struct xMD5Context *ctx, const byte * buf, int len)
{
    word32 t;

    /* Update byte count */

    t = ctx->bytes[0];
    if ((ctx->bytes[0] = t + len) < t)
	ctx->bytes[1]++;	/* Carry from low to high */

    t = 64 - (t & 0x3f);	/* Space available in ctx->in (at least 1) */
    if ((unsigned) t > (unsigned) len) {
	bcopy(buf, (byte *) ctx->in + 64 - (unsigned) t, len);
	return;
    }
    /* First chunk is an odd size */
    bcopy(buf, (byte *) ctx->in + 64 - (unsigned) t, (unsigned) t);
    byteSwap(ctx->in, 16);
    xMD5Transform(ctx->buf, ctx->in);
    buf += (unsigned) t;
    len -= (unsigned) t;

    /* Process data in 64-byte chunks */
    while (len >= 64) {
	bcopy(buf, (byte *) ctx->in, 64);
	byteSwap(ctx->in, 16);
	xMD5Transform(ctx->buf, ctx->in);
	buf += 64;
	len -= 64;
    }

    /* Handle any remaining bytes of data. */
    bcopy(buf, (byte *) ctx->in, len);
}

/*
 * Final wrapup - pad to 64-byte boundary with the bit pattern 
 * 1 0* (64-bit count of bits processed, MSB-first)
 */
static void
xMD5Final(byte digest[16], struct xMD5Context *ctx)
{
    int count = (int) (ctx->bytes[0] & 0x3f);	/* Bytes in ctx->in */
    byte *p = (byte *) ctx->in + count;	/* First unused byte */

    /* Set the first char of padding to 0x80.  There is always room. */
    *p++ = 0x80;

    /* Bytes of padding needed to make 56 bytes (-8..55) */
    count = 56 - 1 - count;

    if (count < 0) {		/* Padding forces an extra block */
	bzero(p, count + 8);
	byteSwap(ctx->in, 16);
	xMD5Transform(ctx->buf, ctx->in);
	p = (byte *) ctx->in;
	count = 56;
    }
    bzero(p, count + 8);
    byteSwap(ctx->in, 14);

    /* Append length in bits and transform */
    ctx->in[14] = ctx->bytes[0] << 3;
    ctx->in[15] = ctx->bytes[1] << 3 | ctx->bytes[0] >> 29;
    xMD5Transform(ctx->buf, ctx->in);

    byteSwap(ctx->buf, 4);
    bcopy((byte *) ctx->buf, digest, 16);
    bzero((byte *) ctx, sizeof(ctx));
}

/* dest buffer MUST be at least 16 bytes long. */
static void
MD5hash(void *dest, const void *orig, int len)
{
    struct xMD5Context context;

    xMD5Init(&context);
    xMD5Update(&context, (const byte *) orig, len);
    xMD5Final((byte *) dest, &context);
}

/*
 * outbuf MUST be at least (((strlen(inbuf)+3)/4)*3)+1 chars long to read
 * the full set of base64 encoded data in the string.  More simply, make sure
 * your output buffer is at least 3/4ths the size of your input, plus 4 bytes.
 */
static size_t
Base64Decode(void *outbuf, size_t outbuflen, const char *inbuf)
{
    unsigned char *outb = (unsigned char *) outbuf;
    const char *in = inbuf;
    unsigned int acc = 0;
    unsigned int val = 0;
    size_t bytcnt = 0;
    int bitcnt = 0;

    while (*in || bitcnt) {
	if (!*in || *in == '=') {
	    val = 0;
	} else if (*in >= 'A' && *in <= 'Z') {
	    val = *in - 'A';
	} else if (*in >= 'a' && *in <= 'z') {
	    val = *in - 'a' + 26;
	} else if (*in >= '0' && *in <= '9') {
	    val = *in - '0' + 52;
	} else if (*in == '+') {
	    val = 62;
	} else if (*in == '/') {
	    val = 63;
	} else {
	    in++;
	    continue;
	}
	acc = (acc << 6) | (val & 0x3f);
	bitcnt += 6;
	if (bitcnt >= 8) {
	    if (bytcnt >= outbuflen) {
		break;
	    }
	    bytcnt++;
	    bitcnt -= 8;
	    *outb++ = (acc >> bitcnt) & 0xff;
	    acc &= ~(0xff << bitcnt);
	}
	if (*in)
	    in++;
    }
    return bytcnt;
}

/*
 * outbuf MUST be at least (((inlen+2)/3)*4)+1 chars long.
 * More simply, make sure your output buffer is at least 4/3rds the size
 * of the input buffer, plus five bytes.
 */
static void
Base64Encode(char *outbuf, const void *inbuf, size_t inlen)
{
    const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

/* dest buffer MUST be at least 24 chars long. */
void
MD5base64(char *dest, const void *orig, int len)
{
    void *tmp = (void *) malloc(16);
    MD5hash(tmp, orig, len);
    Base64Encode(dest, tmp, 16);
    free(tmp);
}

/* Create the initial buffer for the given connection and dump some semi-
   random string into it to start.  If seed is zero, seed off the clock. */
void *
init_seed(char *seed)
{
    word32 *digest;
    int tbuf[8];

    if (!(digest = (word32 *) malloc(sizeof(word32) * 4))) {
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

/* Deletes a buffer. */
void
delete_seed(void *buffer)
{
    free(buffer);
}

word32
rnd(void *buffer)
{
    word32 *digest = (word32 *) buffer;

    if (!digest)
	return (0);
    MD5hash(digest, digest, sizeof(digest));
    return (digest[0]);
}
