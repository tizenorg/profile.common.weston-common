%bcond_with wayland
Name:       weston-common
Version:    1
Release:    0
Summary:    Tizen Common Weston configuration and set-up
License:    MIT
Group:      Base/Configuration
Source0:    %{name}-%{version}.tar.bz2
Source1001: weston-common.manifest
Provides:   weston-startup

Requires:   weston
# for getent:
Requires:   glibc
# for useradd et al
Requires:   pwdutils

BuildRequires:  autoconf >= 2.64, automake >= 1.11
BuildRequires:  libtool >= 2.2
BuildRequires:  libjpeg-devel
BuildRequires:  xz
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-cursor)
BuildRequires:  pkgconfig(wayland-egl)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(egl)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(pixman-1)
BuildRequires:  pkgconfig(cairo)
BuildRequires:  pkgconfig(cairo-egl)
BuildRequires:  pkgconfig(cairo-glesv2)
BuildRequires:  pkgconfig(weston)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(gio-2.0)

%if !%{with wayland}
ExclusiveArch:
%endif

############ tz-launcher
%package tz-launcher
Summary: A small launcher for Wayland compositors

%description tz-launcher
A small launcher for Wayland compositors, which reads .desktop files from paths given on the command line or in a config file, and then displays them graphically.
############ qa-plugin
%package qa-plugin
Summary: A Q&A plugin for Weston

%description qa-plugin
A small Weston plugin, disabled by default, which enables features such as listing surfaces along with positions and coordinates.
############

%description
This package contains Tizen Common configuration and set-up for
the Weston compositor, including systemd unit files.

%package config
Summary:    Tizen Common Weston configuration
Group:      Base/Configuration
%description config
This package contains Tizen Common configuration for the Weston
compositor.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%reconfigure
make %{?_smp_mflags}

%install
%define daemon_user display
%define daemon_group display

#install tz-launcher
%make_install

# install weston service as 'display-manager.service' as it's the one wanted by graphical.target
mkdir -p %{buildroot}%{_unitdir}
install -m 644 display-manager-run.service %{buildroot}%{_unitdir}/display-manager-run.service
install -m 644 display-manager.service %{buildroot}%{_unitdir}/display-manager.service
install -m 644 display-manager.path %{buildroot}%{_unitdir}/display-manager.path

# install Environment file for weston service and weston-user.service
mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
install -m 0644 weston.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/weston
install -m 0644 weston-user.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/weston-user

# install tmpfiles.d(5) conf
mkdir -p %{buildroot}%{_prefix}/lib/tmpfiles.d
install -m 0644 weston_tmpfiles.conf %{buildroot}%{_prefix}/lib/tmpfiles.d/weston.conf

# install weston-user service in user session
mkdir -p %{buildroot}%{_unitdir_user}
install -m 644 weston-user.service %{buildroot}%{_unitdir_user}/

# install weston.sh
mkdir -p %{buildroot}%{_sysconfdir}/profile.d/
install -m 0644 weston.sh %{buildroot}%{_sysconfdir}/profile.d/

# install weston.ini
%define weston_config_dir %{_sysconfdir}/xdg/weston
mkdir -p %{buildroot}%{weston_config_dir}
install -m 0644 weston.ini %{buildroot}%{weston_config_dir}

# Add a rule to ensure the 'display' user has permissions to
# open the graphics device
mkdir -p %{buildroot}%{_sysconfdir}/udev/rules.d
cat >%{buildroot}%{_sysconfdir}/udev/rules.d/99-dri.rules <<'EOF'
SUBSYSTEM=="drm", MODE="0660", GROUP="%{daemon_group}", SECLABEL{smack}="*"
EOF

# user 'display' must own /dev/tty7 for weston to start correctly
cat >%{buildroot}%{_sysconfdir}/udev/rules.d/99-tty.rules <<'EOF'
SUBSYSTEM=="tty", KERNEL=="tty7", OWNER="%{daemon_user}", SECLABEL{smack}="^"
EOF

# user 'display' must also be able to access /dev/input/*
cat >%{buildroot}%{_sysconfdir}/udev/rules.d/99-input.rules <<'EOF'
SUBSYSTEM=="input", MODE="0660", GROUP="input", SECLABEL{smack}="^"
EOF

# install desktop file
mkdir -p %{buildroot}%{_datadir}/applications
install -m 0644 weston-terminal.desktop %{buildroot}%{_datadir}/applications

%pre
# create groups 'display' and 'weston-launch'
getent group %{daemon_group} >/dev/null || %{_sbindir}/groupadd -r -o %{daemon_group}
getent group input >/dev/null || %{_sbindir}/groupadd -r -o input
getent group weston-launch >/dev/null || %{_sbindir}/groupadd -r -o weston-launch

# create user 'display'
getent passwd %{daemon_user} >/dev/null || %{_sbindir}/useradd -r -g %{daemon_group} -d /run/display -s /bin/false -c "Display daemon" %{daemon_user}

# add user 'display' to groups 'weston-launch', 'input' and 'video'
groupmod -A %{daemon_user} weston-launch
groupmod -A %{daemon_user} input
groupmod -A %{daemon_user} video

# setup display manager service
mkdir -p %{_unitdir}/graphical.target.wants/
ln -sf ../display-manager.path  %{_unitdir}/graphical.target.wants/

%postun
rm -f %{_unitdir}/graphical.target.wants/display-manager.path

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%{_unitdir}/display-manager-run.service
%{_unitdir}/display-manager.service
%{_unitdir}/display-manager.path
%config %{_sysconfdir}/sysconfig/*
%{_prefix}/lib/tmpfiles.d/weston.conf
%{_unitdir_user}/weston-user.service
%config %{_sysconfdir}/profile.d/*
%config %{_sysconfdir}/udev/rules.d/*
%{_datadir}/applications/*.desktop

%files config
%manifest %{name}.manifest
%config %{weston_config_dir}/weston.ini

%files tz-launcher
%manifest %{name}.manifest
%defattr(-,root,root)
%license src/COPYING
%{_bindir}/tz-launcher
%{_bindir}/wl-pre

%files qa-plugin
%manifest %{name}.manifest
%defattr(-,root,root)
%license src/COPYING
%{_bindir}/weston-qa-client
%{_libdir}/weston/qa-plugin.so
