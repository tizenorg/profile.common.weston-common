export ELM_ENGINE=wayland_egl
export ECORE_EVAS_ENGINE=wayland_egl
export ELM_THEME=tizen-HD-light

# Make EFL apps use the wayland-based input method.
export ECORE_IMF_MODULE=wayland

# workaround systemd bug in pam_systemd module
if [ "$USER" == "display" ]; then
	export XDG_RUNTIME_DIR=/run/display
else
	export XDG_RUNTIME_DIR=/run/user/$UID
fi

