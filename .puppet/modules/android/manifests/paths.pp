# === Class: android::paths
#
# This class defines the paths used in the Android SDK installation
# and operation.
#
# === Authors
#
# Etienne Pelletier <epelletier@maestrodev.com>
#
# === Copyright
#
# Copyright 2012 MaestroDev, unless otherwise noted.
#
class android::paths{

  $installdir = $android::installdir
  $version    = $android::version
  $ndk_version = $android::ndk_version

  case $::kernel {
    'Linux': {
      $sdk_home     = "${installdir}/android-sdk-linux"
      $distrib_file = "android-sdk_r${version}-linux.tgz"

      $ndk_home     = "${installdir}/android-ndk-r${ndk_version}"
      $ndk_distrib_file = "android-ndk-r${ndk_version}-linux-x86.tar.bz2"
    }
    'Darwin': {
      $sdk_home     = "${installdir}/android-sdk-macosx"
      $distrib_file = "android-sdk_r${version}-macosx.zip"
    }
    default: {
      fail("Unsupported Kernel: ${::kernel} operatingsystem: ${::operatingsystem}")
    }
  }
  
  $source  = "http://dl.google.com/android/${distrib_file}"
  $archive = "${installdir}/${distrib_file}"

  $ndk_source = "http://dl.google.com/android/ndk/${ndk_distrib_file}"
  $ndk_archive = "${installdir}/${ndk_distrib_file}"
}