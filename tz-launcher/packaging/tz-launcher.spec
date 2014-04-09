%bcond_with wayland

Name:           tz-launcher
Version:        0.1
Release:        1
Summary:        Tizen Launcher
License:        LGPLv2+
Group:          Graphics & UI Framework/Wayland Window System
Url:            https://github.com/Tarnyko/tz-launcher.git

Source0:         %name-%version.tar.xz
Source1: 	tz-launcher.manifest
BuildRequires:	autoconf >= 2.64, automake >= 1.11
BuildRequires:  libtool >= 2.2
BuildRequires:  libjpeg-devel
BuildRequires:  xz
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-cursor)
BuildRequires:  pkgconfig(wayland-egl)
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

%description
A small launcher for Wayland compositors.

%prep
%setup -q
cp %{SOURCE1} .

%build
%reconfigure
make %{?_smp_mflags}

%install
%make_install
# install binaries


%files
%manifest %{name}.manifest
%defattr(-,root,root)
%license COPYING
%{_bindir}/tz-launcher

%changelog
