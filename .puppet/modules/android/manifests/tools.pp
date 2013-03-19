# == Class: android::platform_tools
#
# Installs the Android SDK platform tools.
#
# === Authors
#
# Etienne Pelletier <epelletier@maestrodev.com>
#
# === Copyright
#
# Copyright 2012 MaestroDev, unless otherwise noted.
#
class android::tools {

  android::package{ 'tools':
    type => 'tools',
    require => [Android::Package['platform-tools']],
  }

}