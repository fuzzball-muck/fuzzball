/* Needed for USE_SSL */
#include "config.h"

#include "externs.h"
#include "interface_ssl.h"

#ifdef USE_SSL

/** Converts an SSL protocol version string to a version number
 *
 *  Inspired by 'set_protocol_version' in
 *  https://github.com/openssl/openssl/blob/master/test/ssltest.c
 *
 *  @param[in] value Name of SSL protocol version
 *  @returns   Version number if found, SSL_UNSUPPORTED_PROTOCOL if unsupported, or -1 if not found
 */
int ssl_protocol_from_string(const char *value) {
	/* Find the desired version */
	size_t i;
	for (i = 0; i < SSL_PROTOCOLS_SIZE; i++)
		if (strcmp(SSL_PROTOCOLS[i].name, value) == 0)
			return SSL_PROTOCOLS[i].version;

	/* Not found, return failure */
	return -1;
}

/** Sets the minimum SSL protocol version given a version string
 *
 *  If None, no change will be made to the SSL context.  If version string is invalid or unsupported
 *  in this build (see SSL_PROTOCOLS), an error is logged to offer guidance on fixing the problem.
 *
 *  @param[in,out] ssl_ctx     SSL context
 *  @param[in]     min_version Name of minimum required SSL protocol version, or "None"
 *  @returns       1 if successful, otherwise 0
 */
int set_ssl_ctx_min_version(SSL_CTX *ssl_ctx, const char *min_version) {
	int min_version_num = ssl_protocol_from_string(min_version);
	switch (min_version_num) {
		case 0:
			/* None specified, no changes needed */
			return 1;
		case -1:
			log_status("ERROR: Unknown ssl_min_protocol_version '%s'.  Specify one of: %s", min_version, SSL_KNOWN_PROTOCOLS);
			fprintf(stderr, "ERROR: Unknown ssl_min_protocol_version '%s'.  Specify one of: %s\n", min_version, SSL_KNOWN_PROTOCOLS);
			return 0;
			break;
		case SSL_UNSUPPORTED_PROTOCOL:
			log_status("ERROR: ssl_min_protocol_version '%s' not supported by the SSL library in this build.  Specify one of: %s", min_version, SSL_KNOWN_PROTOCOLS);
			fprintf(stderr, "ERROR: ssl_min_protocol_version '%s' not supported by the SSL library in this build.  Specify one of: %s\n", min_version, SSL_KNOWN_PROTOCOLS);
			return 0;
			break;
		default:
			log_status("Requiring SSL protocol version '%s' or higher for encrypted connections", min_version);
#if defined(SSL_CTX_set_min_proto_version)
			/* Added in OpenSSL >= 1.1.0
			   See: https://www.openssl.org/docs/manmaster/ssl/SSL_CTX_set_min_proto_version.html */
			return SSL_CTX_set_min_proto_version(ssl_ctx, min_version_num);
#elif defined(SSL_CTX_set_options)
			/* Easy way not implemented, manually turn off protocols to match the minimum version.
			   FB_*_VERSION macros are guaranteed to be defined regardless of OpenSSL version,
			   either as a real version or SSL_UNSUPPORTED_PROTOCOL, which will never be greater
			   than min_version_num. */
			if (min_version_num > FB_SSL3_VERSION)
				SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_SSLv3);
			if (min_version_num > FB_TLS1_VERSION)
				SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1);
			if (min_version_num > FB_TLS1_1_VERSION)
				SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_1);
			if (min_version_num > FB_TLS1_2_VERSION)
				SSL_CTX_set_options(ssl_ctx, SSL_OP_NO_TLSv1_2);
			/* No need to add newer versions as OpenSSL >= 1.1.0 supports the much-nicer
			   SSL_CTX_set_min_proto_version - see above */
			return 1;
#else
			/* A minimum version was requested, but the SSL library doesn't support specifying that.
			   Error out with some advice. */
			log_status("ERROR: specifying ssl_min_protocol_version is not supported by the SSL library used in this build.  Set to 'None', or maybe use OpenSSL?");
			fprintf(stderr, "specifying ssl_min_protocol_version is not supported by the SSL library used in this build.  Set to 'None', or maybe use OpenSSL?\n");
			return 0;
#endif
			break;
	}
}

/** Records the last SSL error to the log
 *
 *  Ignores irrelevant error conditions to avoid spamming the log, only handling a small subset.
 *
 *  @param[in,out] ssl       SSL structure
 *  @param[in]     ret_value Return value from SSL function call
 */
void ssl_log_error(SSL *ssl, const int ret_value) {
	/* Errors require a valid SSL structure, and errors only occur when ret_value is not 1.
	   Bail out early if these conditions aren't met.
	   See https://www.openssl.org/docs/manmaster/ssl/SSL_accept.html */
	if (!ssl || ret_value == 0)
		return;

	/* Get the error value first for requesting the error reason clears the error */
	int ssl_error_value = SSL_get_error(ssl, ret_value);

#ifdef HAVE_OPENSSL
	/* OpenSSL has support for getting the error reason string... */
	const char *reason_str_buf = ERR_reason_error_string(ERR_get_error());
#else
	/* ...but other SSL libraries might not, so assume an unknown error.  Remove this check if
	   not actually needed. */
	const char *reason_str_buf = NULL;
#endif

	/* In the future, additional logging may be desired.  Just add new cases.
	   See https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html */
	switch (ssl_error_value) {
		case SSL_ERROR_SSL:
			/* Use the specific error message if available, or fall back to a generic error if not
			   available (assumptions could mislead an unwary sysadmin). */
			log_status("SSL: Error negotiating encrypted connection (%s)",
			           (reason_str_buf != NULL) ? reason_str_buf : "unknown error");
			break;
		case SSL_ERROR_SYSCALL:
			/* Use the specific error message if available, or fall back to a generic error if not
			   available (assumptions could mislead an unwary sysadmin). */
			log_status("SSL: Error with input/output of encrypted connection (%s)",
			           (reason_str_buf != NULL) ? reason_str_buf : "unknown error");
			break;
		default:
			/* Don't log by default to avoid spamming the system log */
			break;
	}
}

#endif /* USE_SSL */
