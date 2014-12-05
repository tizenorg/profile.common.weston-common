 /* Copyright 2014 Manuel Bachmann <tarnyko@tarnyko.net> */

#include <stdio.h>
#include <string.h>
#include <wayland-client.h>

#include "qa-client-protocol.h"
struct qa *qa = NULL;
int done = 0;


static void
qa_handle_surface_list (void *data, struct qa *qa,
			const char *list)
{
	printf ("qa-client: SURFACES LIST :\n%s", list);
	done = 1;
}

static const struct qa_listener qa_listener = {
	qa_handle_surface_list
};

static void
registry_handle_global (void *data, struct wl_registry *registry,
		        uint32_t id, const char *interface, uint32_t version)
{
	if (strcmp (interface, "qa") == 0) {
		qa = wl_registry_bind (registry, id,
				       &qa_interface, version);
		printf ("qa-client: registered the \"qa\" interface.\n");
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
	struct wl_display *display = NULL;
	struct wl_registry *registry = NULL;
	int res = 0;

	display = wl_display_connect (NULL);
	if (!display) {
		printf ("qa-client: display error.\n");
		printf ("Did you define XDG_RUNTIME_DIR ?\n");
		return -1;
	}

	registry = wl_display_get_registry (display);
	if (!registry) {
		printf ("qa-client: registry error.\n");
		return -1;
	}

	wl_registry_add_listener (registry, &registry_listener, NULL);

	printf ("Waiting for the \"qa\" interface...\n");
	while (!qa)
		wl_display_roundtrip (display);


	qa_add_listener (qa, &qa_listener, NULL);

	qa_surface_list (qa);

	while ((res != -1) && (done == 0))
		res = wl_display_dispatch (display);


	qa_destroy (qa);
	wl_registry_destroy (registry);
	wl_display_flush (display);
	wl_display_disconnect (display);

	return 0;
}
