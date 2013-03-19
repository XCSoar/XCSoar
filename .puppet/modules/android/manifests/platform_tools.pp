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
class android::platform_tools {

  android::package{ 'platform-tools':
    type => 'platform-tools',
  }

}