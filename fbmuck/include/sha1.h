#ifndef _SHA1_H_
#define _SHA1_H_

/* Just in case the including file doesn't use stdint for uint8_t */
#include <stdint.h>

extern void sha1_init(sha1nfo *s);
extern void sha1_writebyte(sha1nfo *s, uint8_t data);
exterm void sha1_write(sha1nfo *s, const char *data, size_t len);
exterm uint8_t* sha1_result(sha1nfo *s);
extern char *hash2hex(uint8_t *hash);

#endif
