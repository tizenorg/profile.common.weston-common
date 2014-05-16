/* Copyright © 2014 Manuel Bachmann */

#include <glib.h>
#include <gio/gio.h>

#define MAX_DESKTOPFILES 100

int desktopfiles;
gchar **desktoptable[MAX_DESKTOPFILES];

int global_argc;
char **global_argv;


gboolean
file_is_parsable (GFile *file)
{
	gboolean result;

	GKeyFile *keyfile = g_key_file_new ();
	if (g_key_file_load_from_file (keyfile, g_file_get_path(file), G_KEY_FILE_NONE, NULL))
		result = TRUE;
	else
		result = FALSE;

	g_key_file_free (keyfile);
	return result;
}

gboolean
file_is_desktop_file (GFile *file)
{
	gboolean result;

	GKeyFile *keyfile = g_key_file_new ();
	g_key_file_load_from_file (keyfile, g_file_get_path(file), G_KEY_FILE_NONE, NULL);

	if (g_key_file_has_group (keyfile, "Desktop Entry"))
		return TRUE;
	else
		return FALSE;

	g_key_file_free (keyfile);
	return result;
}

gboolean
file_is_desktop_file_for_application (GFile *file, gboolean store_values, gchar ***table)
{
	gboolean result;

	GKeyFile *keyfile = g_key_file_new ();
	g_key_file_load_from_file (keyfile, g_file_get_path(file), G_KEY_FILE_NONE, NULL);

	GError *error = NULL;
	gchar **values = g_new (gchar*, 7);
	values[0] = g_key_file_get_value (keyfile, "Desktop Entry", "Name", &error);
	values[1] = g_key_file_get_value (keyfile, "Desktop Entry", "Comment", NULL);
	values[2] = g_key_file_get_value (keyfile, "Desktop Entry", "Exec", &error);
	values[3] = g_key_file_get_value (keyfile, "Desktop Entry", "Icon", NULL);
	values[4] = g_key_file_get_value (keyfile, "Desktop Entry", "Terminal", NULL);
	values[5] = g_key_file_get_value (keyfile, "Desktop Entry", "Type", &error);
	values[6] = NULL;

	if (!values[1]) values[1] = g_strdup (values[0]);
	if (!values[3]) values[3] = g_strdup (values[0]);
	if (!values[4]) values[4] = g_strdup ("false");

	if (error) {
		result = FALSE;
		g_error_free (error);
		g_strfreev(values);
	} else {
		result = TRUE;
		if (store_values)
			*table = values;
		else
			g_strfreev(values);
	}

	g_key_file_free (keyfile);
	return result;
}


void
tz_launcher_parse_file (GFile *file)
{
	if (!file_is_parsable (file)) {
		g_printerr ("File \"%s\" is not parsable !\n",
		            g_file_get_path(file));
		return;
	}

	if (!file_is_desktop_file (file)) {
		g_printerr ("File \"%s\" is not a .desktop file !\n",
		            g_file_get_path(file));
		return;
	}

	gchar **table;
	if (!file_is_desktop_file_for_application (file, 1, &table)) {
		g_printerr ("File \"%s\" is not a .desktop file for an application !\n",
		            g_file_get_path(file));
		return;
	}

	desktoptable[desktopfiles] = g_strdupv(table);
	desktopfiles++;

	g_strfreev(table);
}


void
tz_launcher_parse_directory (GFile *directory)
{
	GError *error = NULL;
	GFileInfo *info = NULL;

	GFileEnumerator *filelist = g_file_enumerate_children (directory,
	                                                       G_FILE_ATTRIBUTE_STANDARD_NAME,
	                                                       0, NULL, &error);
	if (error) {
		g_printerr ("Failed to enumerate content of directory \"%s\" : %s\n",
		            g_file_get_path(directory), error->message);
		g_error_free (error);
		g_object_unref (filelist);
		return;
	}

	while ((info = g_file_enumerator_next_file (filelist, NULL, &error)) != NULL) {
		if (error) {
			g_printerr ("Failed to get info about element of directory \"%s\" : %s\n",
			            g_file_get_path(directory), error->message);
			g_error_free (error);
			g_object_unref (info);
			continue;
		}

		gchar *path = g_strconcat (g_file_get_path(directory), G_DIR_SEPARATOR_S,
			                       g_file_info_get_name (info), NULL);
		GFile *file = g_file_new_for_path (path);
		g_free (path);

		GFileType type = g_file_info_get_file_type (info);
		switch (type) {
			case G_FILE_TYPE_REGULAR:
			case G_FILE_TYPE_SYMBOLIC_LINK:
			case G_FILE_TYPE_UNKNOWN:
				tz_launcher_parse_file (file);
				break;
			case G_FILE_TYPE_DIRECTORY:
				tz_launcher_parse_directory (file);
				break;
		}
		g_object_unref (file);
		g_object_unref (info);
	}

	g_object_unref (filelist);
}

void
tz_launcher_parse_config_file (GFile *file)
{
	GFileInputStream *istream;
	GDataInputStream *stream;
	char *line;
	GFile *linefile;
	GFileType type;

	istream = g_file_read (file, NULL, NULL);
	stream = g_data_input_stream_new (G_INPUT_STREAM(istream));
	while (line = g_data_input_stream_read_line (stream, NULL, NULL, NULL)) {
		line = g_strstrip (line);
		if ((strcmp (line, "")) && (!g_str_has_prefix (line, "#"))) {
			linefile = g_file_new_for_path (line);
			if (g_file_query_exists (linefile, NULL)) {

				type = g_file_query_file_type (linefile, 0, NULL);
				switch (type) {
				case G_FILE_TYPE_SPECIAL:
					g_printerr ("File \"%s\" is special !\n", line);
					break;
				case G_FILE_TYPE_REGULAR:
				case G_FILE_TYPE_SYMBOLIC_LINK:
				case G_FILE_TYPE_UNKNOWN:
					tz_launcher_parse_file (linefile);
					break;
				case G_FILE_TYPE_DIRECTORY:
					tz_launcher_parse_directory (linefile);
					break;
				}

			}
			g_object_unref (linefile);
		}
		g_free (line);
	}
	g_object_unref (stream);
	g_object_unref (istream);
}


void
parse_args (int argc, char **argv)
{

	int i;
	desktopfiles = 0;
	GFile *desktopfile;

	for (i = 1; i < argc ; i++) {

		if (!strcmp (argv[i], "-c")) {
			desktopfile = g_file_new_for_path (argv[i+1]);
			if (!g_file_query_exists (desktopfile, NULL)) {
				g_printerr ("Config file \"%s\" does not exist !\n", argv[i+1]);
				i++; continue;
			}
			tz_launcher_parse_config_file (desktopfile);
			g_object_unref (desktopfile);
			i++; continue;
		}

		desktopfile = g_file_new_for_path (argv[i]);

		if (!g_file_query_exists (desktopfile, NULL)) {
			g_printerr ("File \"%s\" does not exist !\n", argv[i]);
			continue;
		}

		GFileType type = g_file_query_file_type (desktopfile, 0, NULL);
		switch (type) {
			case G_FILE_TYPE_SPECIAL:
				g_printerr ("File \"%s\" is special !\n", argv[i]);
				break;
			case G_FILE_TYPE_REGULAR:
			case G_FILE_TYPE_SYMBOLIC_LINK:
			case G_FILE_TYPE_UNKNOWN:
				tz_launcher_parse_file (desktopfile);
				break;
			case G_FILE_TYPE_DIRECTORY:
				tz_launcher_parse_directory (desktopfile);
				break;
		}

		g_object_unref (desktopfile);

		if (desktopfiles == MAX_DESKTOPFILES)
			break;
	}
}

static void
sigreload_handler (int s)
{
	parse_args (global_argc, global_argv);

	if (desktopfiles > 0)
		tz_launcher_wl_reload (desktopfiles, desktoptable);
}

int
main (int argc, char **argv)
{
	if (argc < 2) {
		g_print ("Usage : tz-launcher <file1>.desktop <file2>.desktop <directory>...\n"
                         "         or\n"
                         "        tz-launcher -c <list-of-.desktop-files>.conf\n");
		return 0;
	}

	g_type_init ();


	signal (SIGUSR1, sigreload_handler);
	global_argc = argc;
	global_argv = argv;


	parse_args (argc, argv);

	if (desktopfiles > 0)
		tz_launcher_wl_run (desktopfiles, desktoptable);
	else
		g_printerr ("No .desktop files found !\n");


	return 0;
}

