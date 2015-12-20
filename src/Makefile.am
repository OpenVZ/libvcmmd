AM_CPPFLAGS = -I../include $(DBUS_CFLAGS)

lib_LTLIBRARIES = libvcmmd.la

libvcmmd_la_SOURCES = vcmmd.c
libvcmmd_la_LDFLAGS = -version-info 0:0:0
libvcmmd_la_LIBADD = $(DBUS_LIBS)

