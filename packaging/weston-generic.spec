Name:       weston-generic
Version:    1
Release:    0
Summary:    Tizen Generic Weston configuration and set-up
License:    MIT
Group:      Base/Configuration
BuildArch:  noarch
Source0:    %{name}-%{version}.tar.bz2
Source1001: weston-generic.manifest

%description
This package contains Tizen Generic configuration and set-up for
the Weston compositor.

%prep
%setup -q
cp %{SOURCE1001} .

%build

%install

%define weston_config_dir %{_sysconfdir}/xdg/weston
mkdir -p %{buildroot}%{weston_config_dir}
install -m 0644 weston.ini %{buildroot}%{weston_config_dir}

%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%config %{weston_config_dir}/weston.ini
