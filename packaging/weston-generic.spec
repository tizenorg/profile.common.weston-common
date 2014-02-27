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

install -d %{buildroot}%{_unitdir_user}/weston.target.wants
install -m 644 weston.service %{buildroot}%{_unitdir_user}/weston.service
ln -sf ../weston.service %{buildroot}/%{_unitdir_user}/weston.target.wants/

mkdir -p %{buildroot}%{_sysconfdir}/profile.d/
install -m 0644 weston.sh %{buildroot}%{_sysconfdir}/profile.d/

%define weston_config_dir %{_sysconfdir}/xdg/weston
mkdir -p %{buildroot}%{weston_config_dir}
install -m 0644 weston.ini %{buildroot}%{weston_config_dir}

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%{_unitdir_user}/weston.service
%{_unitdir_user}/weston.target.wants/weston.service
%config %{_sysconfdir}/profile.d/*

%files config
%manifest %{name}.manifest
%config %{weston_config_dir}/weston.ini
