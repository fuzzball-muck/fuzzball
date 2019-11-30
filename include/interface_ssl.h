/** @file interface_ssl.h
 *
 * This file contains the declarations for the interface to the SSL library.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef INTERFACE_SSL_H
#define INTERFACE_SSL_H

/* Needed for USE_SSL */
#include "config.h"

#ifdef USE_SSL

/* Needed for descriptor structure */
#include "interface.h"

# ifdef HAVE_OPENSSL
#  include <openssl/ssl.h>
#  include <openssl/err.h>
# else
#  include <ssl.h>
# endif

/*
 * Backwards-compatibility for valid SSL protocol versions that are
 * not supported.
 */
#define SSL_UNSUPPORTED_PROTOCOL -2

/*
 * Build a lookup table to convert protocol version string to integer
 *
 * Unfortunately OpenSSL lacks an easy way of pragmatically determining what
 * protocols are supported at runtime.  Thus, build a reference table
 * according to compile-time constants.  Unavailable protocols are marked with
 * SSL_UNSUPPORTED_PROTOCOL.
 *
 * See 'set_protocol_version' and 'protocol_from_string' in
 * https://github.com/openssl/openssl/blob/OpenSSL_1_1_0-stable/test/ssltest_old.c
 *
 * To further complicate matters, OpenSSL plans to remove support for older SSL
 * versions over time.  Fuzzball needs to provide a fallback to make sure
 * existing setups specifying removed protocol versions will get a useful
 * error message pointing to the problem, instead of entirely failing to
 * compile.
 *
 * FB_PROTOCOL_VERSION  Copy OpenSSL's PROTOCOL_VERSION if defined, otherwise
 *                      SSL_UNSUPPORTED_PROTOCOL
 * FB_PROTOCOL_NAME     Friendly name for error messages, blank if not available
 */
#ifdef SSL3_VERSION
# define FB_SSL3_VERSION SSL3_VERSION
# define FB_SSL3_NAME "SSLv3 "
#else
# define FB_SSL3_VERSION SSL_UNSUPPORTED_PROTOCOL
# define FB_SSL3_NAME ""
#endif
#ifdef TLS1_VERSION
# define FB_TLS1_VERSION TLS1_VERSION
# define FB_TLS1_NAME "TLSv1 "
#else
# define FB_TLS1_VERSION SSL_UNSUPPORTED_PROTOCOL
# define FB_TLS1_NAME ""
#endif
#ifdef TLS1_1_VERSION
# define FB_TLS1_1_VERSION TLS1_1_VERSION
# define FB_TLS1_1_NAME "TLSv1.1 "
#else
# define FB_TLS1_1_VERSION SSL_UNSUPPORTED_PROTOCOL
# define FB_TLS1_1_NAME ""
#endif
#ifdef TLS1_2_VERSION
# define FB_TLS1_2_VERSION TLS1_2_VERSION
# define FB_TLS1_2_NAME "TLSv1.2 "
#else
# define FB_TLS1_2_VERSION SSL_UNSUPPORTED_PROTOCOL
# define FB_TLS1_2_NAME ""
#endif

/* List of protocols supported at compile-time, used for error messages */
#define SSL_KNOWN_PROTOCOLS FB_SSL3_NAME FB_TLS1_NAME FB_TLS1_1_NAME FB_TLS1_2_NAME

/* A single SSL protocol version */
struct ssl_protocol_version {
    const char *name;
    int version;
};

/* Valid SSL protocol versions */
static const struct ssl_protocol_version SSL_PROTOCOLS[] = {
    {"None", 0},
    {"SSLv3", FB_SSL3_VERSION},
    {"TLSv1", FB_TLS1_VERSION},
    {"TLSv1.1", FB_TLS1_1_VERSION},
    {"TLSv1.2", FB_TLS1_2_VERSION}
};

static const size_t SSL_PROTOCOLS_SIZE = (sizeof(SSL_PROTOCOLS) / sizeof(SSL_PROTOCOLS[0]));

/* SSL logging levels */
typedef enum {
    SSL_LOGGING_NONE,
    SSL_LOGGING_ERROR,
    SSL_LOGGING_WARN,
    SSL_LOGGING_DEBUG
} ssl_logging_t;


/**
 * @private
 * @var SSL logging level for connection handling
 */
extern const ssl_logging_t ssl_logging_connect;

/**
 * @private
 * @var SSL logging level for socket reads/writes
 */
extern const ssl_logging_t ssl_logging_stream;

/**
 * Converts an SSL protocol version string to a version number
 *
 * Inspired by 'set_protocol_version' and 'protocol_from_string' in
 * https://github.com/openssl/openssl/blob/OpenSSL_1_1_0-stable/test/ssltest_old.c
 *
 * @param value Name of SSL protocol version
 * @returns Version number if found, SSL_UNSUPPORTED_PROTOCOL if unsupported,
 *          or -1 if not found
 */
int ssl_protocol_from_string(const char *);

/**
 * Sets the minimum and maximum SSL protocol version given a version string
 *
 * If None, no change will be made to the SSL context.  If version string is
 * invalid or unsupported in this build (see SSL_PROTOCOLS), an error is
 * logged to offer guidance on fixing the problem.
 *
 * This will also set the maximum SSL protocol version to 1.2 if FORCE_TLS12
 * is set.
 *
 * @param ssl_ctx      SSL context
 * @param min_version  Name of minimum required SSL protocol version, or "None"
 * @returns boolean true if successful, false otherwise
 */
int set_ssl_ctx_versions(SSL_CTX *, const char *);

/**
 * Checks for the last SSL error, if any, recording it to the log
 *
 * If log_all is set to 0, this ignores any usually irrelevant error
 * conditions to avoid spamming the log, only handling a small subset.
 *
 * @param d          Connection descriptor data
 * @param ret_value  Return value from SSL function call
 * @param log_level  Amount of logging to do according to ssl_logging_t
 * @returns SSL_ERROR_NONE if successful or unknown, otherwise value from
 *          SSL_get_error()
 */
int ssl_check_error(struct descriptor_data *, const int, const ssl_logging_t);

/*
 * SSL_ERROR_WANT_ACCEPT is not defined in OpenSSL v0.9.6i and before. This
 * fix allows to compile fbmuck on systems with an 'old' OpenSSL library, and
 * yet have the server recognize the WANT_ACCEPT error when ran on systems with
 * a newer OpenSSL version installed... Of course, should the value of the
 * define change in ssl.h in the future (unlikely but not impossible), the
 * define below would have to be changed too...
 */
#ifndef SSL_ERROR_WANT_ACCEPT
#define SSL_ERROR_WANT_ACCEPT 8
#endif

#endif /* USE_SSL */
#endif /* !INTERFACE_SSL_H */
