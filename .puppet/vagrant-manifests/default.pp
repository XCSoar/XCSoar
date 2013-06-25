class apt {
  exec { 'apt-get update':
    command => '/usr/bin/apt-get update',
  }
}

class libcurl {
  package { "libcurl4-openssl-dev":
    ensure => present,
  }
}

class libsdl {
  package { "libsdl1.2-dev":
    ensure => present,
  }
}

class libfreetype {
  package { "libfreetype6-dev":
    ensure => present,
  }
}

class gxx {
  package { "g++":
    ensure => present,
  }
}

class java {
  package { "default-jdk":
    ensure => present,
  }
}

class ant {
  package { "ant":
    ensure => present,
  }
}

class mingw-w64 {
  package { "gcc-mingw-w64":
    ensure => present,
  }

  package { "g++-mingw-w64":
    ensure => present,
  }
}

class mingw32ce {
  exec { 'download':
    command => '/usr/bin/wget http://max.kellermann.name/download/xcsoar/devel/cegcc/mingw32ce-mk-2013-04-03-i386.tar.xz',
    cwd => '/home/vagrant/',
    creates => '/home/vagrant/mingw32ce-mk-2013-04-03-i386.tar.xz',
    user => 'vagrant',
  }

  exec { 'extract':
    command => '/bin/tar -xf mingw32ce-mk-2013-04-03-i386.tar.xz',
    cwd => '/home/vagrant/',
    creates => '/home/vagrant/mingw32ce-mk-2013-04-03-i386/bin/arm-mingw32ce-g++',
    user => 'vagrant',
    require => Exec['download'],
  }

  exec { 'add-to-path':
    command => '/bin/echo PATH="/home/vagrant/mingw32ce-mk-2013-04-03-i386/bin:\$PATH" >> .profile',
    cwd => '/home/vagrant/',
    require => Exec['extract'],
    unless => '/bin/grep mingw32ce .profile',
  }
}

class make {
  package { "make":
    ensure => present,
  }
}

class ccache {
  package { "ccache":
    ensure => present,
  }
}

class gettext {
  package { "gettext":
    ensure => present,
  }
}

class xsltproc {
  package { "xsltproc":
    ensure => present,
  }
}

class rsvg {
  package { "librsvg2-bin":
    ensure => present,
  }
}

class imagemagick {
  package { "imagemagick":
    ensure => present,
  }
}

class oggenc {
  package { "vorbis-tools":
    ensure => present,
  }
}

class ubuntu-font {
  package { "ttf-ubuntu-font-family":
    ensure => present,
  }
}

# Stages
stage { ['pre', 'post']: }
Stage['pre'] -> Stage['main'] -> Stage['post']

# Update apt package index
class {'apt': stage => 'pre' }

# Libraries
class {'libcurl': }
class {'libsdl': }
class {'libfreetype': }

# Toolchain
class {'make': }
class {'gxx': }
class {'java': }
class {'ant': }
class {'ccache': }

# MinGW Toolchain
class {'mingw-w64': }

# CeGCC Toolchain
class {'mingw32ce': }

# Resource tools
class {'gettext': }
class {'xsltproc': }
class {'rsvg': }
class {'imagemagick': }
class {'oggenc': }
class {'ubuntu-font': }

# Android Toolchain
class {'android': 
  user => 'vagrant',
  group => 'vagrant',
  installdir => '/home/vagrant/opt',
} -> 
exec { 'android-sdk-link':
  command => '/bin/ln -s android-sdk-linux android-sdk-linux_x86',
  cwd     => '/home/vagrant/opt',
  creates => '/home/vagrant/opt/android-sdk-linux_x86',
} ->
android::platform { 'android-16':
  require => [Android::Package['tools']],
}

include wget

wget::fetch { 'download-ioiolib':
  source      => 'http://git.xcsoar.org/cgit/mirror/ioio.git/snapshot/ioio-0af63173ab3d85b50b6a8cf61e43806657b693f8.tar.gz',
  destination => '/home/vagrant/opt/ioio-0af63173ab3d85b50b6a8cf61e43806657b693f8.tar.gz',
  require => [File['/home/vagrant/opt']],
} ->
exec { 'unpack-ioiolib':
  command => '/bin/tar -xvf ioio-0af63173ab3d85b50b6a8cf61e43806657b693f8.tar.gz --no-same-owner --no-same-permissions',
  creates => '/home/vagrant/opt/ioio-0af63173ab3d85b50b6a8cf61e43806657b693f8',
  cwd     => '/home/vagrant/opt',
} ->
exec { 'ioiolib-link':
  command => '/bin/ln -s ioio-0af63173ab3d85b50b6a8cf61e43806657b693f8 ioiolib',
  cwd     => '/home/vagrant/opt',
  creates => '/home/vagrant/opt/ioiolib',
}
