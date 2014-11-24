/* Copyright © 2014 Manuel Bachmann */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include <glib.h>
#include <wayland-client.h>

#include "window.h"

typedef enum {
	TLM_UI_REQUEST_NONE = 0,
	TLM_UI_REQUEST_LOGIN,
	TLM_UI_REQUEST_LOGOUT,
	TLM_UI_REQUEST_SWITCH_USER
} TlmRequestType;

extern void tlm_dbus_request (TlmRequestType req_type, char *username, char *password);


struct main_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;
	struct wl_list user_list;
};

struct user_entry {
	struct widget *widget;
	int focused, pressed, inactive;
	struct wl_list link;

	char *name;
	cairo_surface_t *icon;
};

struct display *display;
struct main_window *main_window;


cairo_surface_t*
load_icon (char *path)
{
	cairo_t *cr;
	cairo_surface_t *icon;
	char *default_iconpath = DATADIR "/weston/icon_window.png";

	cairo_surface_t *icon_temp;
	cairo_status_t status;
	 /* verify the icons really exist and are loadable */
	icon_temp = cairo_image_surface_create_from_png (path);
	status = cairo_surface_status (icon_temp);

	if (status != CAIRO_STATUS_SUCCESS) {
		icon_temp = cairo_image_surface_create_from_png (default_iconpath);
		status = cairo_surface_status (icon_temp);

		if (status != CAIRO_STATUS_SUCCESS)
			return NULL;
	}

	icon = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
	cr = cairo_create (icon);
	 /* resize the icon to 64x64 */
	int width = cairo_image_surface_get_width(icon_temp);
	int height = cairo_image_surface_get_height(icon_temp);
	if ((width != 64) && (height != 64)) {
		double ratio = ((64.0/width) < (64.0/height) ? (64.0/width) : (64.0/height));
		cairo_scale (cr, ratio, ratio);
	}
	cairo_set_source_surface (cr, icon_temp, 0.0, 0.0);
	cairo_paint (cr);

	cairo_destroy (cr);
	cairo_surface_destroy (icon_temp);

	return icon;
}

static int
user_entry_enter_handler(struct widget *widget, struct input *input,
			 float x, float y, void *data)
{
	struct user_entry *entry = data;

	if (entry->inactive)
		return CURSOR_LEFT_PTR;

	entry->focused = 1;
	widget_schedule_redraw (widget);

	return CURSOR_LEFT_PTR;
}

static void
user_entry_leave_handler(struct widget *widget, struct input *input,
			 void *data)
{
	struct user_entry *entry = data;

	if (entry->inactive)
		return;

	entry->focused = 0;
	widget_schedule_redraw (widget);
}

static void
user_entry_button_handler(struct widget *widget, struct input *input,
			  uint32_t time, uint32_t button,
			  enum wl_pointer_button_state state, void *data)
{
	struct user_entry *entry = data;

	if (entry->inactive)
		return;

	widget_schedule_redraw (widget);

	if (state == WL_POINTER_BUTTON_STATE_RELEASED)
		tlm_dbus_request (TLM_UI_REQUEST_LOGIN, entry->name, "tizen");
}

static void
user_entry_touch_down_handler(struct widget *widget, struct input *input,
			      uint32_t serial, uint32_t time, int32_t id,
			      float x, float y, void *data)
{
	struct user_entry *entry = data;

	if (entry->inactive)
		return;

	entry->focused = 1;
	widget_schedule_redraw (widget);
}

static void
user_entry_touch_up_handler(struct widget *widget, struct input *input,
			    uint32_t serial, uint32_t time, int32_t id,
			    void *data)
{
	struct user_entry *entry = data;

	if (entry->inactive)
		return;

	entry->focused = 0;
	widget_schedule_redraw (widget);

	tlm_dbus_request (TLM_UI_REQUEST_LOGIN, entry->name, "tizen");
}

static void
user_entry_redraw_handler(struct widget *widget,
			  void *data)
{
	struct user_entry *entry = data;
	struct rectangle allocation;
	cairo_t *cr;

	cr = widget_cairo_create (main_window->widget);
	widget_get_allocation (widget, &allocation);

	cairo_set_source_surface (cr, entry->icon,
				  allocation.x, allocation.y);
	cairo_paint (cr);

	cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
	cairo_set_font_size (cr, 20);
	cairo_move_to (cr, allocation.x + 60, allocation.y + 40);
	cairo_show_text (cr, entry->name);

	if (entry->inactive) {
		cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.9);
		cairo_mask_surface (cr, entry->icon,
				    allocation.x, allocation.y);
	}

	if (entry->focused) {
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.4);
		cairo_mask_surface (cr, entry->icon,
				    allocation.x, allocation.y);
	}

	cairo_destroy (cr);
}

static void
main_window_add_user_entry (const char *name)
{
	struct user_entry *entry;

	entry = xzalloc (sizeof *entry);
	entry->icon = load_icon (DATADIR "/tz-greeter/user.png");
	entry->name = strdup (name);

	wl_list_insert (main_window->user_list.prev, &entry->link);

	entry->widget = widget_add_widget (main_window->widget, entry);
	widget_set_enter_handler (entry->widget, user_entry_enter_handler);
	widget_set_leave_handler (entry->widget, user_entry_leave_handler);
	widget_set_button_handler (entry->widget, user_entry_button_handler);
	widget_set_touch_down_handler (entry->widget, user_entry_touch_down_handler);	
	widget_set_touch_up_handler (entry->widget, user_entry_touch_up_handler);
	widget_set_redraw_handler (entry->widget, user_entry_redraw_handler);
}

static void
resize_handler (struct widget *widget, int32_t width, int32_t height,
		void* data)
{
	struct main_window *main_window = data;
	struct user_entry *entry;
	cairo_t *cr;
	cairo_text_extents_t extents;
	int x, y, w, h;

	x = 50;
	y = 100;
	wl_list_for_each (entry, &main_window->user_list, link) {
		cr = cairo_create (entry->icon);
		cairo_text_extents (cr, entry->name, &extents);
		w = cairo_image_surface_get_width (entry->icon) + extents.width + 10;
		h = cairo_image_surface_get_height (entry->icon);
		widget_set_allocation (entry->widget,
				       x, y - h / 2, w + 1, h + 1);
		y += h + 10;
		cairo_destroy (cr);
	}
}

static void
redraw_handler (struct widget *widget, void *data)
{
	struct main_window *main_window = data;
	struct rectangle allocation;
	cairo_surface_t *surface;
	cairo_t *cr;

	widget_get_allocation (main_window->widget, &allocation);

	surface = window_get_surface (main_window->window);
	cr = cairo_create (surface);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
			 allocation.x,
			 allocation.y,
			 allocation.width,
			 allocation.height);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.6);
	cairo_fill (cr);

	cairo_destroy (cr);
}

void
main_window_create ()
{
	struct passwd *pwd;
	int extchars, extwidth, usercount;

	main_window = xzalloc (sizeof *main_window);
	main_window->window = window_create (display);
	main_window->widget = window_frame_create (main_window->window, main_window);
	wl_list_init (&main_window->user_list);

	window_set_title (main_window->window, "Choose a user");

	window_set_user_data (main_window->window, main_window);
	widget_set_redraw_handler (main_window->widget, redraw_handler);
	widget_set_resize_handler (main_window->widget, resize_handler);

	extwidth = 0;
	usercount = 0;
	 /* list valid users here */
	pwd = getpwent ();
	while (pwd != NULL) {
		if ((g_strcmp0 ("x", pwd->pw_passwd) == 0)
		     && (pwd->pw_uid >= 1000) && (pwd->pw_uid <= 60000)
		     && (g_strcmp0 (pwd->pw_shell, "/bin/false") != 0)
		     && (g_strcmp0 (pwd->pw_shell, "/sbin/nologin") != 0)) {
			main_window_add_user_entry (pwd->pw_name);
			if ((extchars = strlen(pwd->pw_name) - 7) > 0) {
				if (extchars > extwidth)
					extwidth = extchars;
			}
			usercount++;
		}
		pwd = getpwent ();
	}
	endpwent ();

	widget_schedule_resize (main_window->widget, 260 + extwidth * 10,
						     200 + (usercount-1) * 68);
}

void
main_window_destroy ()
{
	if (main_window->surface)
		cairo_surface_destroy (main_window->surface);

	struct user_entry *entry, *tmp;
	wl_list_for_each_safe (entry, tmp, &main_window->user_list, link) {
		cairo_surface_destroy (entry->icon);
		widget_destroy (entry->widget);
		wl_list_remove (&entry->link);
		free (entry->name);
		free (entry);
	}

	widget_destroy (main_window->widget);
	window_destroy (main_window->window);
	free (main_window);
}

void
tz_greeter_wl_run ()
{
	display = NULL;
	int retries = 0;

	while (!display) {
		display = display_create (NULL, NULL);
		if (!display) {
			retries++;
			if (retries > 3)
				break;
			sleep (3);
		}
	}
	if (!display) {
		g_printerr ("Failed to connect to a Wayland compositor !\n");
		return;
	}

	main_window_create ();
	display_run (display);

	main_window_destroy ();
	display_destroy (display);
}
