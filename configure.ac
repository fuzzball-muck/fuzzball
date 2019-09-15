dnl
dnl Process this file with autoconf to produce a configure script.
dnl
dnl TinyMUCK fb5.x auto-config script writen by: Peter "WhiteFire" Torkelson
dnl Rewritten for fb6.x by revar@belfry.com
dnl 
AC_REVISION($Revision: 1.30 $)
AC_PREREQ([2.69])
dnl Version numbers work like this:
dnl 9.03, 9.03+master, 9.04-master, 9.04
dnl Release version numbers should only exist for a SINGLE commit, the same commit that gets tagged for release.
dnl Version number changes should always be their own commit, with no functional changes in the same commit.
dnl This means the last 9.04-master commit, the only 9.04 commit, and first 9.04+master commit are functionally the same:
dnl this is because it's just two commits in a row changing nothing but the version number.
AC_INIT([fbmuck],[7.00b1+master],[feedback@fuzzball.org],[fbmuck],[https://www.fuzzball.org/])
AC_CONFIG_AUX_DIR(auto)
AC_CONFIG_HEADER([include/autoconf.h])

echo " "
echo "TinyMUCK fb7.x auto-configure script."
echo " "
echo "This script will try and determine things about your system so"
echo "that FB can compile correctly. This will create your Makefile"
echo "and the header file autoconf.h in the include directory."
echo " "

dnl
dnl Find the compiler first.
dnl
AC_PROG_INSTALL

dnl
dnl Requires autoconf-archive project.
dnl
AX_CHECK_COMPILE_FLAG([-std=gnu99], [CFLAGS="$CFLAGS -std=gnu99"])
AX_CHECK_COMPILE_FLAG([-fwrapv], [CFLAGS="$CFLAGS -fwrapv"])

dnl
dnl Libraries
dnl

checkpcredir() { :
    if test -f "$1/include/pcre/pcre.h"; then
        AC_DEFINE(HAVE_PCREINCDIR, [], [PCRE headers are under pcre dir.])
        pcredir="$1"
        return 0
    fi
    if test -f "$1/include/pcre.h"; then
        pcredir="$1"
        return 0
    fi
    return 1
}

dnl Let the user specify where to find the PCRE libs
AC_ARG_WITH(pcre,
    [  --with-pcre=DIR         location of installed PCRE libraries/include files],
    [
	if test "$withval" = "no"; then
		pcreenable=0
	elif test "$withval" != "yes"; then
		pcreenable=1
		checkpcredir "$withval"
	else
		pcreenable=1
	fi

    ],
    [
	pcreenable=1
    ]
)
if test "$pcreenable" = "1"; then
	AC_MSG_CHECKING([for PCRE directory])
	if test -z "$pcredir"; then
		for maindir in ~ /usr/local /usr/local/opt /usr/pkg /sw /opt/local /opt /usr
		do
			for dir in $maindir $maindir/pcre
			do
				checkpcredir $dir && break 2
			done
		done
	fi
	if test -z "$pcredir"; then
   		AC_MSG_RESULT([Not found])
		echo
		echo "Couldn't find your PCRE library installation dir."
		echo "Use --with-pcre option to fix this problem"
		echo
		exit 1
	fi
	AC_MSG_RESULT([$pcredir])
	AC_SUBST(pcredir)
	AC_DEFINE_UNQUOTED(pcredir, "$pcredir", [The base path to the installation for PCRE.  Usually /usr.])

	if test "$pcredir"; then
		INC="$INC -I$pcredir/include"
		LIBS="$LIBS -L$pcredir/lib -lpcre"
		AC_CHECK_LIB(pcre, pcre_free)
	fi
	AC_SUBST(INC)
else
	echo
	echo "fbmuck requires the PCRE library to compile."
	echo "Use --with-pcre option to fix this problem"
	echo
	exit 1
fi

checkssldir() { :
    if test -f "$1/include/openssl/ssl.h"; then
        AC_DEFINE(HAVE_OPENSSL, [], [ssl headers are under openssl dir.])
        ssldir="$1"
        return 0
    fi
    if test -f "$1/include/ssl.h"; then
        ssldir="$1"
        return 0
    fi
    return 1
}

dnl if the user wants SSL, he has to ask for it
AC_ARG_WITH(ssl,
    [  --with-ssl=DIR          location of installed SSL libraries/include files],
    [
        dnl Check the specified localtion only
		AC_MSG_CHECKING([for SSL directory])
		if test "$withval" = "no"; then
			sslenable=0
		elif test "$withval" != "yes"; then
			sslenable=1
        	checkssldir "$withval"
		else
			sslenable=1
			for maindir in ~ /usr/local /usr/local/opt /usr/pkg /sw /opt/local /opt /usr /var/ssl
			do
				for dir in $maindir $maindir/openssl $maindir/ssl
				do
					checkssldir $dir && break 2
				done
			done
		fi

    ],
    [
        dnl THE USER DID NOT ASK FOR SSL, DON'T CHECK IT!!!
		sslenable=0
    ]
)
if test "$sslenable" = "1"; then
	if test -z "$ssldir"; then
   		AC_MSG_RESULT([Not found])
    	echo
    	echo "Couldn't find your SSL library installation dir."
    	echo "Use --with-ssl option to fix this problem, if you want ssl support"
    	echo
	exit 1
	fi
	AC_MSG_RESULT([$ssldir])
	AC_SUBST(ssldir)
	AC_DEFINE_UNQUOTED(ssldir, "$ssldir", [The base path to the installation for openssl.  Usually /usr.])

	if test "$ssldir"; then
		INC="$INC -I$ssldir/include"
		LIBS="$LIBS -L$ssldir/lib -lssl"
		AC_CHECK_LIB(crypto, CRYPTO_free)
		AC_CHECK_LIB(ssl, SSL_read)
	fi
	AC_SUBST(INC)
fi

AC_ARG_ENABLE(memory-cleanup,
[  --enable-memory-cleanup        free all memory on exit (for memory debugging tools)],
[
if test "$enableval" = yes; then
	AC_DEFINE(MEMORY_CLEANUP, [], [Enable freeing memory on exit.])
fi
])

AC_ARG_ENABLE(memprof,
[  --enable-memprof           enable memory leak detection and profiling (slow)],
[
if test "$enableval" = yes; then
	AC_DEFINE(MALLOC_PROFILING)
        AC_DEFINE(MEMORY_CLEANUP)
else
	if test "$enableval" = debug; then
		AC_DEFINE(MALLOC_PROFILING, [], [Enables memory usage profiling.])
		AC_DEFINE(MALLOC_PROFILING_EXTRA, [], [With MALLOC_PROFILING, can detect double-frees, buffer overruns, etc.])
	fi
fi
])

AC_ARG_ENABLE(debug,
[  --enable-debug             enable developer assertions],
[
if test "$enableval" = yes; then
	AC_DEFINE(DEBUG, [], [Enable debugging assertions.])
fi
])

AC_ARG_ENABLE(force-tls12,
[  --enable-force-tls12              force TLS 1.2],
[
if test "$enableval" = yes; then
	AC_DEFINE(FORCE_TLS12, [], [Force TLS 1.2.])
fi
])

dnl
dnl Header files
dnl
AC_CHECK_HEADERS(malloc.h)
AC_HEADER_DIRENT

dnl
dnl Types and structures
dnl
AC_TYPE_PID_T
AC_TYPE_SIZE_T

AC_CHECK_FUNCS(mallinfo getrlimit getrusage arc4random_uniform pselect)
AC_CHECK_MEMBER([struct mallinfo.hblks])
AC_CHECK_MEMBER([struct mallinfo.keepcost])
AC_CHECK_MEMBER([struct mallinfo.treeoverhead])
AC_CHECK_MEMBER([struct mallinfo.grain])
AC_CHECK_MEMBER([struct mallinfo.allocated])

dnl
dnl Remove HAVE_CONFIG_H
dnl
AC_CONFIG_COMMANDS_PRE([DEFS=])

dnl And in the end, there was no more.
dnl
AC_CONFIG_FILES([Makefile
	src/Makefile
	game/restart
])
AC_OUTPUT
echo " "
echo "You should review the options in include/config.h, and"
echo "then type make to build your system."
echo " "
