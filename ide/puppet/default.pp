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
class {'ccache': }

# Resource tools
class {'xsltproc': }
class {'rsvg': }
class {'imagemagick': }
