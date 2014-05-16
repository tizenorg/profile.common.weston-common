/* Copyright © 2014 Manuel Bachmann */

#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>
#include <wayland-client.h>

#include "window.h"


struct main_window {
	struct window *window;
	struct widget *widget;
	cairo_surface_t *surface;
	struct wl_list launcher_list;
	struct launcher *selected_launcher;

	char *user;
};

struct launcher {
	struct widget *widget;
	int focused, pressed;
	struct wl_list link;

	char *name;
	char *comment;
	char *exec;
	cairo_surface_t *icon;
	gboolean terminal;
};

struct main_window *main_window;


cairo_surface_t*
load_icon_from_themes (char *iconpath, char *path)
{
	GError *error = NULL;
	GFile *directory = NULL;
	GFile *index = NULL;
	GFileInfo *info = NULL;
	gchar *indexpath;
	cairo_surface_t *icon_temp = NULL;

	if ((!iconpath) || (!path))
		return NULL;

	 /* take a shortcut to the icons if we are in a theme directory */
	indexpath = g_strconcat (path, G_DIR_SEPARATOR_S, "index.theme", NULL);
	index = g_file_new_for_path (indexpath);
	if (g_file_query_exists (index, NULL)) {
		g_object_unref (index);
		g_free (indexpath);
		indexpath = g_strconcat (path, G_DIR_SEPARATOR_S, "32x32/apps", NULL);
		index = g_file_new_for_path (indexpath);
		if (g_file_query_exists (index, NULL))
			icon_temp = load_icon_from_themes (iconpath, indexpath);
		g_object_unref (index);
		g_free (indexpath);

		if (icon_temp)
			return icon_temp;
		else
			return NULL;
	}
	g_object_unref (index);
	g_free (indexpath);

	directory = g_file_new_for_path (path);
	GFileEnumerator *filelist = g_file_enumerate_children (directory,
	                                                       G_FILE_ATTRIBUTE_STANDARD_NAME,
	                                                       0, NULL, &error);
	if (error) {
		g_printerr ("Failed to enumerate content of theme directories !\n");
		g_error_free (error);
		g_object_unref (filelist);
		g_object_unref (directory);
		return NULL;
	}

	while ((info = g_file_enumerator_next_file (filelist, NULL, &error)) != NULL) {
		if (error) {
			g_printerr ("Failed to enumerate content of theme directories !\n");
			g_error_free (error);
			g_object_unref (info);
			continue;
		}

		gchar *subpath = g_strconcat (g_file_get_path(directory), G_DIR_SEPARATOR_S,
			                          g_file_info_get_name (info), NULL);

		GFileType type = g_file_info_get_file_type (info);
		switch (type) {
			case G_FILE_TYPE_REGULAR:
				if ((!strcmp (iconpath, g_file_info_get_name (info))) ||
				    (!strcmp (g_strconcat (iconpath, ".png", NULL), g_file_info_get_name (info)))) {
					icon_temp = cairo_image_surface_create_from_png (subpath);
					return icon_temp;
				}
				break;
			case G_FILE_TYPE_DIRECTORY:
				icon_temp = load_icon_from_themes (iconpath, subpath);
				if (icon_temp)
					return icon_temp;
				break;
		}
		g_free (subpath);
		g_object_unref (info);
	}

	g_object_unref (filelist);
	g_object_unref (directory);

	return NULL;
}

cairo_surface_t*
load_icon (char *path)
{
	cairo_t *cr;
	cairo_surface_t *icon;
	gchar *iconpath;
	char *default_iconpath = DATADIR "/weston/icon_window.png";
	
	if (path)
		iconpath = g_strdup (path);
	else
		iconpath = g_strdup (default_iconpath);

	cairo_surface_t *icon_temp;
	cairo_status_t status;
	 /* verify the icons really exist and are loadable */
	icon_temp = cairo_image_surface_create_from_png (iconpath);
	status = cairo_surface_status (icon_temp);
	if (status != CAIRO_STATUS_SUCCESS) {
		icon_temp = load_icon_from_themes (iconpath, "/usr/share/icons");
		if (icon_temp)
			status = cairo_surface_status (icon_temp);

		if (status != CAIRO_STATUS_SUCCESS) {
			iconpath = g_strdup (default_iconpath);
			icon_temp = cairo_image_surface_create_from_png (iconpath);
			status = cairo_surface_status (icon_temp);

			if (status != CAIRO_STATUS_SUCCESS) {
				g_printerr ("Could not find default icon \"%s\", exiting !\n", default_iconpath);
				g_free (default_iconpath);
				g_free (iconpath);
				g_thread_exit (NULL);
			}
		}
	}

	icon = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 32, 32);
	cr = cairo_create (icon);
	 /* resize the icon to 32x32 */
	int width = cairo_image_surface_get_width(icon_temp);
	int height = cairo_image_surface_get_height(icon_temp);
	if (width != height != 32) {
		double ratio = ((32.0/width) < (32.0/height) ? (32.0/width) : (32.0/height));
		cairo_scale (cr, ratio, ratio);
	}
	cairo_set_source_surface (cr, icon_temp, 0.0, 0.0);
	cairo_paint (cr);

	cairo_destroy (cr);
	cairo_surface_destroy (icon_temp);
	g_free (iconpath);

	return icon;
}

static void
sigchild_handler (int s)
{
	pid_t pid;

	while (pid = waitpid (-1, NULL, WNOHANG), pid > 0)
		g_printerr ("child %d exited\n", pid);
}

static void
launcher_button_handler(struct widget *widget,
			      struct input *input, uint32_t time,
			      uint32_t button,
			      enum wl_pointer_button_state state, void *data)
{
	struct launcher *launcher = data;

	widget_schedule_redraw (widget);
	if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
			if (fork () == 0) {
				gchar **command = g_strsplit (launcher->exec, " ", 0);
				execvp (command[0], command);
				g_strfreev (command);
			}
	}
}

static void
launcher_touch_up_handler(struct widget *widget, struct input *input,
			     uint32_t serial, uint32_t time, int32_t id,
			     float x, float y, void *data)
{
	struct launcher *launcher = data;

	launcher->focused = 0;
	widget_schedule_redraw(widget);

	if (fork () == 0) {
		gchar **command = g_strsplit (launcher->exec, " ", 0);
		execvp (command[0], command);
		g_strfreev (command);
	}
}

static void
launcher_touch_down_handler(struct widget *widget, struct input *input,
			     uint32_t serial, uint32_t time, int32_t id,
			     float x, float y, void *data)
{
	struct launcher *launcher = data;

	launcher->focused = 1;
	widget_schedule_redraw(widget);

	main_window->selected_launcher = launcher;
}

static int
launcher_enter_handler(struct widget *widget, struct input *input,
			     float x, float y, void *data)
{
	struct launcher *launcher = data;

	launcher->focused = 1;
	widget_schedule_redraw (widget);

	main_window->selected_launcher = launcher;

	return CURSOR_LEFT_PTR;
}

static void
launcher_leave_handler(struct widget *widget,
			     struct input *input, void *data)
{
	struct launcher *launcher = data;

	launcher->focused = 0;
	widget_destroy_tooltip (widget);
	widget_schedule_redraw (widget);

	main_window->selected_launcher = NULL;
}

static int
launcher_motion_handler(struct widget *widget, struct input *input,
			      uint32_t time, float x, float y, void *data)
{
	struct launcher *launcher = data;

	widget_set_tooltip (widget, launcher->comment, x, y);

	return CURSOR_LEFT_PTR;
}

static void
launcher_redraw_handler(struct widget *widget, void *data)
{
	struct launcher *launcher = data;
	struct rectangle allocation;
	cairo_t *cr;

	cr = widget_cairo_create(main_window->widget);

	widget_get_allocation(widget, &allocation);
	if (launcher->pressed) {
		allocation.x++;
		allocation.y++;
	}

	cairo_set_source_surface(cr, launcher->icon,
				 allocation.x, allocation.y);
	cairo_paint(cr);

	if (launcher->focused) {
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
		cairo_mask_surface(cr, launcher->icon,
				   allocation.x, allocation.y);
	}

	cairo_destroy(cr);
}

void
main_window_add_launcher (gchar **desktopentry)
{
	struct launcher *launcher;
	launcher = xzalloc(sizeof *launcher);

	launcher->name = g_strdup (desktopentry[0]);
	launcher->comment = g_strdup (desktopentry[1]);
	launcher->exec = g_strdup (desktopentry[2]);
	launcher->icon = load_icon (desktopentry[3]);

	if (strcmp("true", g_ascii_strdown(desktopentry[4],-1)) == 0)
		launcher->terminal = TRUE;
	else
		launcher->terminal = FALSE;

	wl_list_insert(main_window->launcher_list.prev, &launcher->link);

	launcher->widget = widget_add_widget(main_window->widget, launcher);
	widget_set_enter_handler(launcher->widget, launcher_enter_handler);
	widget_set_leave_handler(launcher->widget, launcher_leave_handler);
	widget_set_motion_handler(launcher->widget, launcher_motion_handler);
	widget_set_button_handler(launcher->widget, launcher_button_handler);
	widget_set_touch_down_handler(launcher->widget, launcher_touch_down_handler);
	widget_set_touch_up_handler(launcher->widget, launcher_touch_up_handler);
	widget_set_redraw_handler(launcher->widget, launcher_redraw_handler);
}

static void
resize_handler(struct widget *widget, int32_t width, int32_t height, void *data)
{
	struct main_window *main_window = data;
	struct launcher *launcher;
	int x, y, w, h, i;

	x = 40; y = 60; i = 0;
	wl_list_for_each(launcher, &main_window->launcher_list, link) {
		w = cairo_image_surface_get_width(launcher->icon);
		h = cairo_image_surface_get_height(launcher->icon);
		widget_set_allocation(launcher->widget,
				      x, y, w + 1, h + 1);
		x += w + 10;

		i++;
		if (i % (width/45) == 0)
			{ x = 40; y += h + 10; }
	}
}

static void
redraw_handler(struct widget *widget, void *data)
{
	struct main_window *main_window = data;
	struct rectangle allocation;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_text_extents_t extents;
	double red, green, blue;

	widget_get_allocation (main_window->widget, &allocation);

	surface = window_get_surface (main_window->window);
	cr = cairo_create(surface);
	cairo_set_operator( cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr,
			allocation.x,
			allocation.y,
			allocation.width,
			allocation.height);

	red = green = blue = 0.0;
	switch (getuid() % 10) {
		case 0: red = green = blue = 0.0; break;
		case 1: red = 1.0; break;
		case 2: blue = 1.0; break;
		case 3: green = 1.0; break;
		case 9: red = green = 1.0; break;
		default: red = blue = 1.0; break;
	}

	cairo_set_source_rgba (cr, red, green, blue, 0.5);
	cairo_fill (cr);

	if (main_window->selected_launcher) {
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
		cairo_text_extents (cr, main_window->selected_launcher->name, &extents);
		cairo_move_to (cr, allocation.x + (allocation.width - extents.width)/2,
	    	               allocation.y + (allocation.height - extents.height));
		cairo_show_text (cr, main_window->selected_launcher->name);
	}
}

void
main_window_create (struct display *display, int desktopfiles, gchar ***desktoptable)
{
	main_window = xzalloc (sizeof *main_window);
	main_window->window = window_create (display);
	main_window->widget = window_frame_create (main_window->window, main_window);
	wl_list_init (&main_window->launcher_list);

	int i;
	for (i = 0; i < desktopfiles; i++) {
		main_window_add_launcher (desktoptable[i]);
		g_strfreev (desktoptable[i]);
	}

	if (getenv("USER"))
		main_window->user = g_strdup (getenv("USER"));
	else
		main_window->user = g_strdup ("(Unknown)");

	gchar *title = g_strconcat ("Tizen Launcher : ", main_window->user, NULL);
	window_set_title (main_window->window, title);
	g_free (title);

	window_set_user_data(main_window->window, main_window);
	widget_set_redraw_handler(main_window->widget, redraw_handler);
	widget_set_resize_handler(main_window->widget, resize_handler);

	int rows = 0;
	int margin = desktopfiles - 18;
	if (margin > 0)
		rows = ((margin/9 * 48) + 48);
	widget_schedule_resize(main_window->widget, 480, 200 + rows);
}

void
main_window_destroy ()
{
	if (main_window->surface)
		cairo_surface_destroy (main_window->surface);

	struct launcher *launcher, *tmp;
	wl_list_for_each_safe(launcher, tmp, &main_window->launcher_list, link) {
		cairo_surface_destroy (launcher->icon);
		widget_destroy(launcher->widget);
		wl_list_remove(&launcher->link);
		free (launcher->name);
		free (launcher->comment);
		free (launcher->exec);
		free (launcher);
	}

	widget_destroy (main_window->widget);
	window_destroy (main_window->window);
	free (main_window->user);
	free (main_window);
}

void
tz_launcher_wl_run (int desktopfiles, gchar ***desktoptable)
{
	struct display *display = NULL;
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
		int i;
		for (i = 0; i < desktopfiles; i++)
			g_strfreev (desktoptable[i]);
		return;
	}

	signal (SIGCHLD, sigchild_handler);

	main_window_create (display, desktopfiles, desktoptable);
	display_run (display);

	main_window_destroy ();
	display_destroy (display);
}
