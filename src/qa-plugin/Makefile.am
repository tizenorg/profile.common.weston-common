bin_PROGRAMS =
moduledir = $(libdir)/weston
module_LTLIBRARIES =

all-local :

AM_CFLAGS = $(GCC_CFLAGS)

AM_CPPFLAGS =

CLEANFILES =

BUILT_SOURCES =					\
	qa-protocol.c				\
	qa-server-protocol.h			\
	qa-client-protocol.h

qa-protocol.c : qa.xml
	$(wayland_scanner) code < $< > $@
qa-server-protocol.h : qa.xml
	$(wayland_scanner) server-header < $< > $@
qa-client-protocol.h : qa.xml
	$(wayland_scanner) client-header < $< > $@

bin_PROGRAMS += weston-qa-client

weston_qa_client_LDFLAGS = -export-dynamic
weston_qa_client_CPPFLAGS = $(AM_CPPFLAGS)
weston_qa_client_CFLAGS = $(GCC_CFLAGS) $(CLIENT_CFLAGS)
weston_qa_client_LDADD = $(DLOPEN_LIBS) $(CLIENT_LIBS)

weston_qa_client_SOURCES =				\
	qa-client.c					\
	qa-protocol.c

module_LTLIBRARIES += qa-plugin.la

qa_plugin_la_LDFLAGS = -module -avoid-version
qa_plugin_la_CPPFLAGS = $(AM_CPPFLAGS) -Wno-unused-result
qa_plugin_la_CFLAGS = $(GCC_CFLAGS) $(SERVER_CFLAGS) $(PIXMAN_CFLAGS)
qa_plugin_la_LIBADD = $(DLOPEN_LIBS) $(SERVER_LIBS) $(PIXMAN_LIBS)

qa_plugin_la_SOURCES =					\
	qa-plugin.c					\
	qa-protocol.c
