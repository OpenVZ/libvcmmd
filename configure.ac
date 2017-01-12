AC_INIT([libvcmmd], [7.0.20])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# If we are on an x86_64, redefine libdir from default value
if test "$target_cpu" = "x86_64" -a "$libdir" = '${exec_prefix}/lib'; then
	if debarch=$(dpkg-architecture -qDEB_HOST_MULTIARCH 2>/dev/null); then
		# Debian/Ubuntu
		libdir='${exec_prefix}/'$debarch
	else
		# Assume Red Hat/SUSE: libs go to lib64
		libdir='${exec_prefix}/lib64'
	fi
fi

AM_INIT_AUTOMAKE([foreign dist-bzip2])

AC_DISABLE_STATIC

AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_LN_S

PKG_CHECK_MODULES([DBUS], [dbus-1])

CFLAGS="${CFLAGS} -Wall -Werror"

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
