#ifndef _SHA1_H_
#define _SHA1_H_

/* Just in case the including file doesn't use stdint for uint8_t */
#include <stdint.h>

/* Size of the hash and size of block */
#define HASH_LENGTH 20
#define BLOCK_LENGTH 64

typedef struct sha1nfo {
	uint32_t buffer[BLOCK_LENGTH / 4];
	uint32_t state[HASH_LENGTH / 4];
	uint32_t byteCount;
	uint8_t bufferOffset;
	uint8_t keyBuffer[BLOCK_LENGTH];
	uint8_t innerHash[HASH_LENGTH];
} sha1nfo;

extern void sha1_init(sha1nfo *s);
extern void sha1_writebyte(sha1nfo *s, uint8_t data);
extern void sha1_write(sha1nfo *s, const char *data, size_t len);
extern uint8_t* sha1_result(sha1nfo *s);
extern void hash2hex(uint8_t* hash, char *buffer, size_t buflen);

#endif
