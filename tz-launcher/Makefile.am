bin_PROGRAMS =

all-local :

AM_CFLAGS = $(GCC_CFLAGS)

AM_CPPFLAGS = 					\
	-I$(top_srcdir)/toytoolkit		\
	-DDATADIR='"$(datadir)"'

CLEANFILES =

bin_PROGRAMS += tz-launcher wl-pre

tz_launcher_LDFLAGS = -export-dynamic
tz_launcher_CPPFLAGS = $(AM_CPPFLAGS)
tz_launcher_CFLAGS = $(GCC_CFLAGS) $(PNG_CFLAGS) $(PIXMAN_CFLAGS) $(CLIENT_CFLAGS) $(CAIRO_EGL_CFLAGS) $(GLIB_CFLAGS)
tz_launcher_LDADD = $(DLOPEN_LIBS) $(PNG_LIBS) $(PIXMAN_LIBS) $(CLIENT_LIBS) $(CAIRO_EGL_LIBS) $(JPEG_LIBS) $(GLIB_LIBS) -lm

tz_launcher_SOURCES =					\
	tz-launcher.c					\
	tz-launcher-wl.c				\
	toytoolkit/shared/frame.c			\
	toytoolkit/shared/image-loader.c		\
	toytoolkit/shared/cairo-util.c			\
	toytoolkit/shared/os-compatibility.c		\
	toytoolkit/xdg-shell-protocol.c			\
	toytoolkit/text-cursor-position-protocol.c	\
	toytoolkit/workspaces-protocol.c		\
	toytoolkit/window.c

wl_pre_LDFLAGS = -export-dynamic
wl_pre_CFLAGS = $(GCC_CFLAGS) $(CLIENT_CFLAGS)
wl_pre_LDADD = $(DLOPEN_LIBS) $(CLIENT_LIBS)

wl_pre_SOURCES = wl-pre.c
