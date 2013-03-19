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

  package { "libsdl-image1.2-dev":
    ensure => present,
  }

  package { "libsdl-ttf2.0-dev":
    ensure => present,
  }

  # Workaround for broken libsdl-ttf2.0-dev package on Ubuntu 12.04
  exec { 'SDL_ttf.pc':
    command => '/bin/sed \'s/SDL_image/SDL_ttf/g\' /usr/lib/i386-linux-gnu/pkgconfig/SDL_image.pc > /usr/lib/i386-linux-gnu/pkgconfig/SDL_ttf.pc',
    require => [Package['libsdl-ttf2.0-dev'], Package['libsdl-image1.2-dev']],
    creates => '/usr/lib/i386-linux-gnu/pkgconfig/SDL_ttf.pc',
  }
}

class libboost {
  package { "libboost-dev":
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

# Stages
stage { ['pre', 'post']: }
Stage['pre'] -> Stage['main'] -> Stage['post']

# Update apt package index
class {'apt': stage => 'pre' }

# Libraries
class {'libcurl': }
class {'libsdl': }
class {'libboost': }

# Toolchain
class {'make': }
class {'gxx': }
class {'java': }
class {'ant': }
class {'ccache': }

# MinGW Toolchain
class {'mingw-w64': }

# Resource tools
class {'gettext': }
class {'xsltproc': }
class {'rsvg': }
class {'imagemagick': }
class {'oggenc': }

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
android::platform { 'android-16': }

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
