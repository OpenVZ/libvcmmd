#!/bin/sh
#

libtoolize --automake --copy --force
aclocal
automake --add-missing --copy --force
autoconf
rm -rf autom4te.cache/
