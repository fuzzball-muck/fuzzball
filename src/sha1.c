#include <stddef.h>
#include <stdint.h>

#include "config.h"

#include "sha1.h"

/* Test for endian options -- Windows is LITTLE by default */
#ifdef __BIG_ENDIAN__
# define SHA_BIG_ENDIAN
#elif defined __LITTLE_ENDIAN__
#elif defined __BYTE_ORDER
# if __BYTE_ORDER__ ==  __ORDER_BIG_ENDIAN__
# define SHA_BIG_ENDIAN
# endif
#elif defined WIN32
#else				// ! defined __LITTLE_ENDIAN__
# include <endian.h>		// machine/endian.h
# if __BYTE_ORDER__ ==  __ORDER_BIG_ENDIAN__
#  define SHA_BIG_ENDIAN
# endif
#endif

/* K values for SHA1 */
#define SHA1_K0  0x5a827999
#define SHA1_K20 0x6ed9eba1
#define SHA1_K40 0x8f1bbcdc
#define SHA1_K60 0xca62c1d6

/* Starting values */
void
sha1_init(sha1nfo * s)
{
    s->state[0] = 0x67452301;
    s->state[1] = 0xefcdab89;
    s->state[2] = 0x98badcfe;
    s->state[3] = 0x10325476;
    s->state[4] = 0xc3d2e1f0;
    s->byteCount = 0;
    s->bufferOffset = 0;
}

/* Roll bits */
static uint32_t
sha1_rol32(uint32_t number, uint8_t bits)
{
    return ((number << bits) | (number >> (32 - bits)));
}

/* Hash the data in the block */
static void
sha1_hashBlock(sha1nfo * s)
{
    uint32_t a, b, c, d, e, t;

    a = s->state[0];
    b = s->state[1];
    c = s->state[2];
    d = s->state[3];
    e = s->state[4];
    for (uint8_t i = 0; i < 80; i++) {
	if (i >= 16) {
	    t = s->buffer[(i + 13) & 15] ^ s->buffer[(i + 8) & 15] ^ s->buffer[(i +
										2) & 15] ^ s->
		    buffer[i & 15];
	    s->buffer[i & 15] = sha1_rol32(t, 1);
	}
	if (i < 20) {
	    t = (d ^ (b & (c ^ d))) + SHA1_K0;
	} else if (i < 40) {
	    t = (b ^ c ^ d) + SHA1_K20;
	} else if (i < 60) {
	    t = ((b & c) | (d & (b | c))) + SHA1_K40;
	} else {
	    t = (b ^ c ^ d) + SHA1_K60;
	}
	t += sha1_rol32(a, 5) + e + s->buffer[i & 15];
	e = d;
	d = c;
	c = sha1_rol32(b, 30);
	b = a;
	a = t;
    }
    s->state[0] += a;
    s->state[1] += b;
    s->state[2] += c;
    s->state[3] += d;
    s->state[4] += e;
}

/* Used to add single bytes of data in case of padding */
static void
sha1_addUncounted(sha1nfo * s, uint8_t data)
{
    uint8_t *const b = (uint8_t *) s->buffer;
#ifdef SHA_BIG_ENDIAN
    b[s->bufferOffset] = data;
#else
    b[s->bufferOffset ^ 3] = data;
#endif
    s->bufferOffset++;
    if (s->bufferOffset == BLOCK_LENGTH) {
	sha1_hashBlock(s);
	s->bufferOffset = 0;
    }
}

/* Write a single byte of data into the buffer */
void
sha1_writebyte(sha1nfo * s, uint8_t data)
{
    ++s->byteCount;
    sha1_addUncounted(s, data);
}

/* Write a length of data into the buffer */
void
sha1_write(sha1nfo * s, const char *data, size_t len)
{
    for (; len--;)
	sha1_writebyte(s, (uint8_t) * data++);
}

/* Pad if not enough data when finalized */
static void
sha1_pad(sha1nfo * s)
{
    // Implement SHA-1 padding
    // Pad with 0x80 followed by 0x00 until the end of the block
    sha1_addUncounted(s, 0x80);
    while (s->bufferOffset != 56)
	sha1_addUncounted(s, 0x00);

    // Append length in the last 8 bytes
    sha1_addUncounted(s, 0);	// We're only using 32 bit lengths
    sha1_addUncounted(s, 0);	// But SHA-1 supports 64 bit lengths
    sha1_addUncounted(s, 0);	// So zero pad the top bits
    sha1_addUncounted(s, s->byteCount >> 29);	// Shifting to multiply by 8
    sha1_addUncounted(s, s->byteCount >> 21);	// as SHA-1 supports bitstreams as well as
    sha1_addUncounted(s, s->byteCount >> 13);	// byte.
    sha1_addUncounted(s, s->byteCount >> 5);
    sha1_addUncounted(s, s->byteCount << 3);
}

/* Get the sha1 result */
uint8_t *
sha1_result(sha1nfo * s)
{
    // Pad to complete the last block
    sha1_pad(s);

#ifndef SHA_BIG_ENDIAN
    // Swap byte order back
    for (int i = 0; i < 5; i++) {
	s->state[i] = (((s->state[i]) << 24) & 0xff000000)
		| (((s->state[i]) << 8) & 0x00ff0000)
		| (((s->state[i]) >> 8) & 0x0000ff00)
		| (((s->state[i]) >> 24) & 0x000000ff);
    }
#endif

    // Return pointer to hash (20 characters)
    return (uint8_t *) s->state;
}

/* Turn a 20 byte hash into a 41 byte hex c-string */
void
hash2hex(uint8_t * hash, char *buffer, size_t buflen)
{
    uint8_t low, high;

    buffer[0] = '\0';
    for (size_t i = 0; i < 20; i++) {
	/* Make sure the buffer has two bytes + null */
	if (i * 2 + 3 > buflen)
	    break;
	high = (hash[i] & 0xF0) >> 4;
	low = hash[i] & 0x0F;
	buffer[i * 2] = (char)(high <= 9 ? high + 48 : high + 87);
	buffer[i * 2 + 1] = (char)(low <= 9 ? low + 48 : low + 87);
	buffer[i * 2 + 2] = '\0';
    }
}
