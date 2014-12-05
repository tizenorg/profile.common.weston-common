 /* Copyright 2014 Manuel Bachmann <tarnyko@tarnyko.net> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server.h>

#include <weston/compositor.h>
#include "qa-server-protocol.h"

struct weston_compositor *ec = NULL;



static void
qa_surface_list (struct wl_client *client,
		 struct wl_resource *resource)
{
	struct weston_view *view;
	char *resp, *temp;

	weston_log ("qa-plugin: requested surfaces list...\n");

	resp = strdup ("");

	wl_list_for_each (view, &ec->view_list, link) { printf ("RHA ");
		if ((view->surface) &&
		    (view->geometry.x != 0.0) &&
		    (view->geometry.y != 0.0)) {
			asprintf (&temp, "Surface %p : X+Y = %.2f+%.2f - WxH = %dx%d\n",
				  view->surface, view->geometry.x, view->geometry.y,
				  view->surface->width, view->surface->height);
			resp = realloc (resp, strlen(resp) + strlen (temp) + 1);
			strncat (resp, temp, strlen(temp));
			free (temp);
		}
	}

	qa_send_list_surface (resource, resp);
	free (resp);
}

static void
qa_destroy (struct wl_client *client,
	    struct wl_resource *resource)
{
	wl_resource_destroy (resource);
}

static const struct qa_interface qa_implementation = {
	qa_surface_list,
	qa_destroy
};

static void
bind_qa (struct wl_client *client, void *data,
	 uint32_t version, uint32_t id)
{
	struct wl_resource *resource;

	resource = wl_resource_create (client, &qa_interface,
				       1, id);
	wl_resource_set_implementation (resource, &qa_implementation,
					NULL, NULL);
}

WL_EXPORT int
module_init (struct weston_compositor *compositor,
	     int *argc, char *argv[])
{
	ec = compositor;

	weston_log ("qa-plugin: initialization.\n");

	if (wl_global_create (ec->wl_display, &qa_interface,
			      1, NULL, bind_qa) == NULL)
	{
		weston_log ("qa-plugin: could not bind the \"qa\" interface, exiting...\n");
		return -1;
	}

	return 0;
}
