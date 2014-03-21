%bcond_with wayland
Name:       weston-generic
Version:    1
Release:    0
Summary:    Tizen Generic Weston configuration and set-up
License:    MIT
Group:      Base/Configuration
BuildArch:  noarch
Source0:    %{name}-%{version}.tar.bz2
Source1001: weston-generic.manifest
Provides:   weston-startup

%if !%{with wayland}
ExclusiveArch:
%endif

%description
This package contains Tizen Generic configuration and set-up for
the Weston compositor, including systemd unit files.

%package config
Summary:    Tizen Generic Weston configuration
Group:      Base/Configuration
%description config
This package contains Tizen Generic configuration for the Weston
compositor.

%prep
%setup -q
cp %{SOURCE1001} .

%build

%install

# install weston service as 'display-manager.service' as it's the one wanted by graphical.target
mkdir -p %{buildroot}%{_unitdir}
install -m 644 display-manager-run.service %{buildroot}%{_unitdir}/display-manager-run.service
install -m 644 display-manager.service %{buildroot}%{_unitdir}/display-manager.service
install -m 644 display-manager.path %{buildroot}%{_unitdir}/display-manager.path

# install Environment file for weston service
mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
install -m 0644 weston.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/weston

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
SUBSYSTEM=="drm", MODE="0660", GROUP="display"
EOF

# user 'display' must own /dev/tty1 for weston to start correctly
cat >%{buildroot}%{_sysconfdir}/udev/rules.d/99-tty.rules <<'EOF'
SUBSYSTEM=="tty", KERNEL=="tty1", GROUP="display", OWNER="display"
EOF

%pre
# create groups 'display' and 'weston-launch'
getent group display >/dev/null || %{_sbindir}/groupadd -r -o display
getent group weston-launch >/dev/null || %{_sbindir}/groupadd -r -o weston-launch

# create user 'display'
getent passwd display >/dev/null || %{_sbindir}/useradd -r -g display -G weston-launch -d /run/display -s /bin/false -c "Display daemon" display

# setup display manager service
mkdir -p %{_unitdir}/graphical.target.wants/
ln -s ../display-manager.path  %{_unitdir}/graphical.target.wants/

# setup display manager access (inside user session)
mkdir -p %{_unitdir_user}/default.target.wants/
ln -s ../weston-user.service  %{_unitdir_user}/default.target.wants/

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%{_unitdir}/display-manager-run.service
%{_unitdir}/display-manager.service
%{_unitdir}/display-manager.path
%config %{_sysconfdir}/sysconfig/weston
%{_prefix}/lib/tmpfiles.d/weston.conf
%{_unitdir_user}/weston-user.service
%config %{_sysconfdir}/profile.d/*
%config %{_sysconfdir}/udev/rules.d/*

%files config
%manifest %{name}.manifest
%config %{weston_config_dir}/weston.ini
