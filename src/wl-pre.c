#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <wayland-client.h>


int main (int argc, char *argv[])
{
        if (argc != 2) {
                printf ("Usage : wl-pre \"<command line>\"\n");
                return 0;
        }

	struct wl_display *display = NULL;

	while (!display) {
		display = wl_display_connect (NULL);
		sleep (1);
	}

	char *command;
	asprintf (&command, "%s &", argv[1]);
	system (command);
	free (command);

	return 0;
}
