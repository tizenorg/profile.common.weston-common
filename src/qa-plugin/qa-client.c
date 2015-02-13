 /* Copyright 2014 Manuel Bachmann <tarnyko@tarnyko.net> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include "qa-client-protocol.h"
struct qa *qa = NULL;
int done = 0;


static void
qa_handle_surface_list (void *data, struct qa *qa,
			const char *list)
{
	printf ("SURFACES LIST :\n%s", list);
	done = 1;
}

static void
qa_handle_surface_move (void *data, struct qa *qa)
{
	printf ("SURFACE MOVE...\n");
	done = 1;
}

static const struct qa_listener qa_listener = {
	qa_handle_surface_list,
	qa_handle_surface_move
};

static void
registry_handle_global (void *data, struct wl_registry *registry,
		        uint32_t id, const char *interface, uint32_t version)
{
	if (strcmp (interface, "qa") == 0) {
		qa = wl_registry_bind (registry, id,
				       &qa_interface, version);
		printf ("weston-qa-client: registered the \"qa\" interface.\n\n");
	}
}

static void
registry_handle_global_remove (void *data, struct wl_registry *registry,
			       uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};


int main (int argc, char *argv[])
{
	if (((argc != 2) && (argc != 5)) ||
	    ((argc == 2) && (strcmp(argv[1],"--help") == 0)) ||
	    ((argc == 2) && (strcmp(argv[1],"--list") != 0)) ||
	    ((argc == 5) && (strcmp(argv[1],"--move") != 0))) {
		printf ("Usage : weston-qa-client --list : list displayed surfaces\n");
		printf ("        weston-qa-client --move <ID> <x> <y> : move surface to position\n");
		printf ("        weston-qa-client --help : this help section\n\n");
		return 0;
	}

	struct wl_display *display = NULL;
	struct wl_registry *registry = NULL;
	int res = 0;

	display = wl_display_connect (NULL);
	if (!display) {
		printf ("weston-qa-client: display error.\n");
		printf ("Did you define XDG_RUNTIME_DIR ?\n");
		return -1;
	}

	registry = wl_display_get_registry (display);
	if (!registry) {
		printf ("weston-qa-client: registry error.\n");
		return -1;
	}

	wl_registry_add_listener (registry, &registry_listener, NULL);

	printf ("Waiting for the \"qa\" interface...\n");
	while (!qa)
		wl_display_roundtrip (display);


	qa_add_listener (qa, &qa_listener, NULL);

	if ((argc == 2) && (strcmp(argv[1],"--list") == 0)) {
		qa_surface_list (qa);
	} else if ((argc == 5) && (strcmp(argv[1],"--move") == 0)) {
		qa_surface_move (qa, atoi(argv[2]),
		                     atoi(argv[3]),
		                     atoi(argv[4]));
	}

	while ((res != -1) && (done == 0))
		res = wl_display_dispatch (display);


	qa_destroy (qa);
	wl_registry_destroy (registry);
	wl_display_flush (display);
	wl_display_disconnect (display);

	return 0;
}
