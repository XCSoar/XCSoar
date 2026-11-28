# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright The XCSoar Project

# OBS workers have no network.  Renovate updates these URLs with the
# pins in build/libboost.mk and build/python/build/libs.py.
# build/download.py verifies both archives against the Make build pins.
#!RemoteAsset
Source0:        https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.bz2
#!RemoteAsset: c3c02c29c0b519de7bd4e25b376e606e87f0761befd1282815642a2246613d14
Source1:        https://github.com/zxing-cpp/zxing-cpp/releases/download/v3.1.1/zxing-cpp-3.1.1.tar.gz
# Package-level assets: the Debian AppImage build (debian/rules) uses these.
#!RemoteAsset
Source2:        https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-x86_64.AppImage
#!RemoteAsset
Source3:        https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64
#!RemoteAsset
Source4:        https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-aarch64.AppImage
#!RemoteAsset
Source5:        https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-aarch64
#!RemoteAsset
Source6:        https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20251107-1/linuxdeploy-armhf.AppImage
#!RemoteAsset
Source7:        https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-armhf

Name:           xcsoar
Version:        7.47
Release:        1%{?dist}
Summary:        Tactical glide computer

License:        GPL-2.0-or-later
URL:            https://xcsoar.org/

Requires:       ca-certificates
Requires:       (dejavu-fonts or (dejavu-sans-fonts and dejavu-sans-mono-fonts))
Suggests:       vali-xcs

BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  binutils
BuildRequires:  cmake
BuildRequires:  pkgconfig
BuildRequires:  python3
BuildRequires:  ImageMagick
BuildRequires:  bzip2
BuildRequires:  quilt
BuildRequires:  sox
%if 0%{?suse_version}
BuildRequires:  gettext-tools
BuildRequires:  hicolor-icon-theme
BuildRequires:  libxslt-tools
BuildRequires:  rsvg-convert
Requires:       hicolor-icon-theme
%else
BuildRequires:  gettext
BuildRequires:  librsvg2-tools
BuildRequires:  libxslt
# Fedora OBS has multiple providers for curl and NetCDF dependencies.
BuildRequires:  openssl-devel
BuildRequires:  hdf-libs
%endif
BuildRequires:  pkgconfig(alsa)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(egl)
BuildRequires:  pkgconfig(fmt) >= 7
BuildRequires:  pkgconfig(freetype2)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(libcares)
BuildRequires:  pkgconfig(libcurl)
BuildRequires:  pkgconfig(libgeotiff)
BuildRequires:  pkgconfig(libjpeg)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libsodium)
BuildRequires:  pkgconfig(libtiff-4)
BuildRequires:  (pkgconfig(lua5.4) or pkgconfig(lua) >= 5.4)
BuildRequires:  pkgconfig(netcdf)
BuildRequires:  pkgconfig(proj)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-cursor)
BuildRequires:  pkgconfig(wayland-egl)
BuildRequires:  pkgconfig(wayland-protocols) >= 1.31
BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:  pkgconfig(zlib)
BuildRequires:  pkgconfig(wayland-scanner)

%description
XCSoar is a tactical glide computer for soaring and paragliding
pilots.

%package -n vali-xcs
Summary:        IGC G-record validation for XCSoar

%description -n vali-xcs
This package provides vali-xcs, which validates signatures generated
by XCSoar.

%prep
# scmsync publishes the git tree as loose sources, not a tarball.
# An in-place build already has the Makefile beside this spec.
if [ ! -f Makefile ]; then
%setup -q -c -T
cp -a %{_sourcedir}/* .
rm -f %{name}.spec _scmsync.obsinfo
fi

%build
# Release builds enable identical code folding, which selects the gold
# linker. Fedora no longer ships gold, so keep the system linker.
make %{?_smp_mflags} TARGET=WAYLAND DEBUG=n ICF=n all mo

%install
make TARGET=WAYLAND DEBUG=n ICF=n install-bin install-mo \
    install-desktop DESTDIR=%{buildroot}
%find_lang %{name}

%files -f %{name}.lang
%license COPYING
%{_bindir}/xcsoar
%{_datadir}/applications/xcsoar.desktop
%{_datadir}/icons/hicolor/scalable/apps/xcsoar.svg

%files -n vali-xcs
%{_bindir}/vali-xcs

%changelog
* Tue Nov 24 2026 XCSoar Project <maintainer@xcsoar.org> - 7.47-1
- Package the Wayland target
