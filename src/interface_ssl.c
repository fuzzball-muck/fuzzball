/** @file interface_ssl.c
 *
 * This file contains the definitions for the interface to the SSL library.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include "config.h"

#ifdef USE_SSL

#include "interface_ssl.h"
#include "log.h"

/*
 * Check if config.h specifies to log all SSL errors
 * @TODO: This should be configurable at runtime, e.g. with an '@debug set
 *        ssl_logging' command.
 */
#ifdef DEBUG_SSL_LOG_ALL
/**
 * @private
 * @var SSL logging level for connection handling
 */
const ssl_logging_t ssl_logging_connect = SSL_LOGGING_DEBUG;

/**
 * @private
 * @var SSL logging level for socket reads/writes
 */
const ssl_logging_t ssl_logging_stream = SSL_LOGGING_DEBUG;

#else
/**
 * @private
 * @var SSL logging level for connection handling
 */
const ssl_logging_t ssl_logging_connect = SSL_LOGGING_WARN;

/**
 * @private
 * @var SSL logging level for socket reads/writes
 */
const ssl_logging_t ssl_logging_stream = SSL_LOGGING_NONE;
#endif /* DEBUG_SSL_LOG_ALL */

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
int
ssl_protocol_from_string(const char *value)
{
    /* Find the desired version */
    for (size_t i = 0; i < SSL_PROTOCOLS_SIZE; i++)
        if (strcmp(SSL_PROTOCOLS[i].name, value) == 0)
            return SSL_PROTOCOLS[i].version;

    /* Not found, return failure */
    return -1;
}

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
int
set_ssl_ctx_versions(SSL_CTX * ssl_ctx, const char *min_version)
{
    int min_version_num = ssl_protocol_from_string(min_version);

#ifdef FORCE_TLS12
    if (min_version_num > TLS1_2_VERSION) {
        min_version_num = TLS1_2_VERSION;
    }
#   if defined(SSL_CTX_set_max_proto_version)
        /*
         * This probably does the same thing as the above if statement, but
         * I would rather be slightly redundant and certain than leave
         * things to chance. (tanabi)
         */
        SSL_CTX_set_max_proto_version(ssl_ctx, TLS1_2_VERSION);
#   endif
#endif

    switch (min_version_num) {
        case 0:
            /* None specified, no changes needed */
            return 1;
        case -1:
            log_status(
                "ERROR: Unknown ssl_min_protocol_version '%s'.  "
                "Specify one of: %s", min_version, SSL_KNOWN_PROTOCOLS);
            fprintf(stderr,
                    "ERROR: Unknown ssl_min_protocol_version '%s'.  "
                    "Specify one of: %s\n", min_version, SSL_KNOWN_PROTOCOLS);
            return 0;
        case SSL_UNSUPPORTED_PROTOCOL:
            log_status("ERROR: ssl_min_protocol_version '%s' not supported "
                       "by the SSL library in this build.  Specify one of: %s",
                       min_version, SSL_KNOWN_PROTOCOLS);
            fprintf(stderr,
                    "ERROR: ssl_min_protocol_version '%s' not supported by "
                    "the SSL library in this build.  Specify one of: %s\n",
                    min_version, SSL_KNOWN_PROTOCOLS);
            return 0;
        default:
            log_status("Requiring SSL protocol version '%s' or higher for "
                       "encrypted connections", min_version);
#if defined(SSL_CTX_set_min_proto_version)
        /*
         * Added in OpenSSL >= 1.1.0
         * See: https://www.openssl.org/docs/man1.1.0/ssl/SSL_CTX_set_min_proto_version.html
         */
        return SSL_CTX_set_min_proto_version(ssl_ctx, min_version_num);
#elif defined(SSL_CTX_set_options)
        /*
         * Easy way not implemented, manually turn off protocols to match the
         * minimum version.  FB_*_VERSION macros are guaranteed to be defined
         * regardless of OpenSSL version, either as a real version or
         * SSL_UNSUPPORTED_PROTOCOL, which will never be greater than
         * min_version_num.
         */
        if (min_version_num > FB_SSL3_VERSION)
            SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);

        if (min_version_num > FB_TLS1_VERSION)
            SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1);

        if (min_version_num > FB_TLS1_1_VERSION)
            SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_1);

#ifndef FORCE_TLS12
        /* We never want to turn this off if we are forcing TLS 1.2 */
        if (min_version_num > FB_TLS1_2_VERSION)
            SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_2);
#endif

        /*
         * No need to add newer versions as OpenSSL >= 1.1.0 supports the
         * much-nicer SSL_CTX_set_min_proto_version - see above
         */
        return 1;
#else
        /*
         * A minimum version was requested, but the SSL library doesn't support specifying that.
         * Error out with some advice.
         */
        log_status("ERROR: specifying ssl_min_protocol_version is not "
                   "supported by the SSL library used in this build.  "
                   "Set to 'None', or maybe use OpenSSL?"
        );

        fprintf(stderr,
                "specifying ssl_min_protocol_version is not supported by the "
                "SSL library used in this build.  Set to 'None', or maybe use "
                "OpenSSL?\n"
        );

        return 0;
#endif
    }
}

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
int
ssl_check_error(struct descriptor_data *d, const int ret_value, const ssl_logging_t log_level)
{
    /*
     * Errors require a valid SSL structure, and errors only occur when
     * ret_value is not 1.  Bail out early if these conditions aren't met.
     * See https://www.openssl.org/docs/man1.1.0/ssl/SSL_accept.html
     */
    if (!d->ssl_session || ret_value == 1) {
        return SSL_ERROR_NONE;
    }

    /*
     * Get the error value first; otherwise, requesting the error reason clears
     * the error
     */
    int ssl_error_value = SSL_get_error(d->ssl_session, ret_value);

    /*
     * Only log if logging is enabled, avoiding any performance impact when
     * the value's not used
     */
    if (log_level != SSL_LOGGING_NONE) {
#ifdef HAVE_OPENSSL
        /* OpenSSL has support for getting the error reason string... */
        const char *reason_str_buf = ERR_reason_error_string(ERR_get_error());

        /*
         * Use the specific error message if available, or fall back to a
         * generic error if not available
         *
         * (assumptions could mislead an unwary sysadmin)
         */
        if (reason_str_buf == NULL) {
            reason_str_buf = "unknown reason";
        }
#else
        /*
         * ...but other SSL libraries might not, so assume an unknown error.
         * Remove this check if not actually needed.
         */
        const char *reason_str_buf = "unknown reason";
#endif

        /* Handle each possible case for SSL logging */
        switch (ssl_error_value) {
            case SSL_ERROR_NONE:
                /*
                 * No error, no need to log anything.  This shouldn't happen,
                 * but just in case...
                 */
                break;

            /*
             * Errors that usually mean bad things happened
             */
            case SSL_ERROR_SSL:
                if (log_level < SSL_LOGGING_ERROR)
                    break;

                /*
                 * These are logged even when SSL protocol considers it
                 * intentional.  This allows tracking when clients connect
                 * that don't support the latest protocols (e.g. when SSLv3 is
                 * disabled).
                 */
                log_status("SSL: Error negotiating encrypted connection "
                           "(%s, SSL_ERROR_SSL) on descriptor %d from %s(%s)",
                           reason_str_buf, d->descriptor, d->hostname,
                           d->username);
                break;

            case SSL_ERROR_SYSCALL:
                if (log_level < SSL_LOGGING_ERROR)
                    break;

                log_status("SSL: Error with input/output of connection (%s, "
                           "SSL_ERROR_SYSCALL) on descriptor %d from %s(%s)",
                           reason_str_buf, d->descriptor, d->hostname,
                           d->username);
                break;

            case SSL_ERROR_ZERO_RETURN:
                if (log_level < SSL_LOGGING_ERROR)
                    break;

                log_status("SSL: Error connection is already closed (%s, "
                           "SSL_ERROR_ZERO_RETURN) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;

            /*
             * Errors that might occur during normal operation (only logged at
             * DEBUG or higher)
             */
            case SSL_ERROR_WANT_READ:
                if (log_level < SSL_LOGGING_DEBUG)
                    break;

                log_status("SSL: Error pending read operation, retry later "
                           "(%s, SSL_ERROR_WANT_READ) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;

            case SSL_ERROR_WANT_WRITE:
                if (log_level < SSL_LOGGING_DEBUG)
                    break;

                log_status("SSL: Error pending write operation, retry later "
                           "(%s, SSL_ERROR_WANT_WRITE) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;

            case SSL_ERROR_WANT_X509_LOOKUP:
                if (log_level < SSL_LOGGING_DEBUG)
                    break;

                log_status("SSL: Error pending X509 lookup, retry later (%s, "
                           "SSL_ERROR_WANT_X509_LOOKUP) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;

            case SSL_ERROR_WANT_CONNECT:
                if (log_level < SSL_LOGGING_DEBUG)
                    break;

                log_status("SSL: Error pending connection, retry later (%s, "
                           "SSL_ERROR_WANT_CONNECT) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;

            case SSL_ERROR_WANT_ACCEPT:
                if (log_level < SSL_LOGGING_DEBUG)
                    break;

                log_status("SSL: Error pending connection accept, retry later "
                           "(%s, SSL_ERROR_WANT_ACCEPT) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;

            /*
             * Unknown errors - something bad happened, or it's a version of
             * SSL with new error messages
             */
            default:
                if (log_level < SSL_LOGGING_WARN)
                    break;

                log_status("SSL: Unknown error (%s) on descriptor %d from "
                           "%s(%s)", reason_str_buf, d->descriptor,
                           d->hostname, d->username);
                break;
        }
    }

    /* Pass on the SSL error value so the calling function can use it */
    return ssl_error_value;
}

#endif /* USE_SSL */
