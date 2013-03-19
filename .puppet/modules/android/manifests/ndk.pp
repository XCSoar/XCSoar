# == Class: android::sdk
#
# This downloads and unpacks the Android SDK. It also
# installs necessary 32bit libraries for 64bit Linux systems.
#
# === Authors
#
# Etienne Pelletier <epelletier@maestrodev.com>
#
# === Copyright
#
# Copyright 2012 MaestroDev, unless otherwise noted.
#
class android::ndk {
  include android::paths
  include wget

  case $::kernel {
    'Linux': {
      $unpack_command = "/bin/tar -xvf ${android::paths::ndk_archive} --no-same-owner --no-same-permissions"
    }
    default: {
      fail("Unsupported Kernel: ${::kernel} operatingsystem: ${::operatingsystem}")
    }
  }

  wget::fetch { 'download-androidndk':
    source      => $android::paths::ndk_source,
    destination => $android::paths::ndk_archive,
    require => [File[$android::paths::installdir]],
  } ->
  exec { 'unpack-androidndk':
    command => $unpack_command,
    creates => $android::paths::ndk_home,
    cwd     => $android::paths::installdir,
    user    => $android::user,
  }
}