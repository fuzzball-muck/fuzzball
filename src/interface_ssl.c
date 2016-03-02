/* Needed for USE_SSL */
#include "config.h"

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

#endif /* USE_SSL */
