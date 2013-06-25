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

  package { "freetype2":
    ensure => present,
  }
}

class gxx {
  package { "g++":
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

# Stages
stage { ['pre', 'post']: }
Stage['pre'] -> Stage['main'] -> Stage['post']

# Update apt package index
class {'apt': stage => 'pre' }

# Libraries
class {'libcurl': }
class {'libsdl': }

# Toolchain
class {'make': }
class {'gxx': }
class {'ccache': }

# MinGW Toolchain
class {'mingw-w64': }

# Resource tools
class {'gettext': }
class {'xsltproc': }
class {'rsvg': }
class {'imagemagick': }
