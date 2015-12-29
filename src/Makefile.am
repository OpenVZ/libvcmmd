AM_CPPFLAGS = -I../include $(DBUS_CFLAGS)

lib_LTLIBRARIES = libvcmmd.la

libvcmmd_la_SOURCES = vcmmd.c
libvcmmd_la_LDFLAGS = -version-info 2:0:1
libvcmmd_la_LIBADD = $(DBUS_LIBS)

