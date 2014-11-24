/* Copyright © 2014 Manuel Bachmann */

#include <unistd.h>

#include <gio/gio.h>

#include "tlm/tlm-dbus-login-gen.h"
#include "tlm/tlm-dbus-utils.h"

typedef enum {
	TLM_UI_REQUEST_NONE = 0,
	TLM_UI_REQUEST_LOGIN,
	TLM_UI_REQUEST_LOGOUT,
	TLM_UI_REQUEST_SWITCH_USER
} TlmRequestType;

extern void tz_greeter_wl_run (void);


void
tlm_dbus_request (TlmRequestType req_type, char *username, char *password)
{
	const gchar *addr;
	GDBusConnection *conn;
	TlmDbusLogin *login;
	GHashTable *env;
	GVariant *venv;
	GError *error = NULL;

	if (getuid () == 0)
		addr = g_strdup_printf ("unix:path=/var/run/tlm/dbus-sock");
	else
		addr = g_strdup_printf ("unix:path=/var/run/tlm/seat1-%d", getuid ());

	conn = g_dbus_connection_new_for_address_sync (addr,
			 G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT,
			 NULL, NULL, &error);
	if (error) {
		g_print ("Could not connect to D-Bus : %s\n", addr);
		g_error_free (error);
	}
	g_free ((gchar *) addr);


	login = tlm_dbus_login_proxy_new_sync (conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
					       "/org/tizen/Tlm/Login", NULL, &error);
	if (error) {
		g_print ("Could not login to the D-Bus ubterface\n");
		g_error_free (error);
	}


	env = g_hash_table_new_full ((GHashFunc) g_str_hash,
	                             (GEqualFunc) g_str_equal,
	                             (GDestroyNotify) g_free,
	                             (GDestroyNotify) g_free);
	venv = tlm_dbus_utils_hash_table_to_variant (env);
	g_hash_table_unref (env);

	if (req_type == TLM_UI_REQUEST_LOGIN) {
		tlm_dbus_login_call_login_user_sync (login, "seat2",
						     username, password,
						     venv, NULL, &error);
	} else if (req_type == TLM_UI_REQUEST_LOGOUT) {
		tlm_dbus_login_call_logout_user_sync (login, "seat2",
						      NULL, &error);
	}
	if (error) {
		g_print ("Could not login or logout user %s\n", username);
		g_error_free (error);
	}


	g_object_unref (login);
	g_object_unref (conn);
}


int
main (int argc, char **argv)
{
	tz_greeter_wl_run (); 

	return 0;
}
