Name:           xcsoar
Version:        6.7.5
Release:        1%{?dist}
Summary:        The open-source glide computer
License:        AGPLv3, LGPLv2, BSD, BSL, MIT
URL:            http://www.xcsoar.org/
Source0:        http://download.xcsoar.org/releases/6.7.5/source/xcsoar-6.7.5.tar.bz2
BuildRequires:  pkgconfig SDL-devel librsvg2-tools libcurl-devel
BuildRequires:  freetype-devel libpng-devel libxslt perl(Data::Dumper) perl(Locale::PO)
BuildRequires:  libjpeg-turbo-devel ImageMagick gettext
BuildRequires:  texlive-scheme-full
Group: Applications/Multimedia

%description 
XCSoar is a tactical glide computer originally
developed for the Pocket PC platform. In 2005,
the originally commercial software was given to
the open-source community for further development
and has constantly been improved since. It is now
a multi-platform application that currently runs
on Windows, Windows Mobile, Unix and even Android
devices. 

%package docs
Summary: Extra documentation for XCSoar
Group: Applications/Multimedia
Requires: %{name}%{?_isa} = %{version}-%{release}
# Just for more intuitive documentation installation
Provides: %{name}-doc = %{version}-%{release}

%description docs
The xcsoar-docs package contains some additional documentation for
XCSoar. Currently, this includes the main documentation in PDF format.

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
%make_install
%find_lang %{name} --all-name
install -pm644 COPYING %{buildroot}%{_pkgdocdir}/COPYING
install -pm644 AUTHORS  %{buildroot}%{_pkgdocdir}/AUTHORS


%files -f %{name}.lang
%{_bindir}/vali-xcs
%{_bindir}/xcsoar
%dir %{_pkgdocdir}
%{_pkgdocdir}/COPYING 
%{_pkgdocdir}/AUTHORS 

%files docs
%doc
%{_pkgdocdir}/XCSoar-*.pdf


%changelog
* Mon Sep 22 2014 Jozef Mlich <jmlich@redhat.com> 6.7.5-1
- Initial version of the package
