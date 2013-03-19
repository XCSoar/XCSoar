# == Define: android::package
#
# This defined resource is used to install Android SDK packages
#
# === Parameters
#
# [*type*] One of platform-tools, platform or addon. Indicates
# the type of package to install.
#
# === Authors
#
# Etienne Pelletier <epelletier@maestrodev.com>
#
# === Copyright
#
# Copyright 2012 MaestroDev, unless otherwise noted.
#
define android::package($type) {
  include android

  $proxy_host = $android::proxy_host ? { undef => '', default => "--proxy-host ${android::proxy_host}" }
  $proxy_port = $android::proxy_port ? { undef => '', default => "--proxy-port ${android::proxy_port}" }

  case $type {
    'platform-tools': {
      $creates = "${android::paths::sdk_home}/platform-tools"
    }
    'tools': {
      $creates = "${android::paths::sdk_home}/tools/jobb"
    }
    'platform': {
      $creates = "${android::paths::sdk_home}/platforms/${title}"
    }
    'addon': {
      $creates = "${android::paths::sdk_home}/add-ons/${title}"
    }
    default: {
      fail("Unsupported package type: ${type}")
    }


  }

  exec { "update-android-package-${title}":
    command => "${android::paths::sdk_home}/tools/android update sdk -u -t ${title} ${proxy_host} ${proxy_port}",
    creates => $creates,
    timeout => 0,
    require => Class['Android::Sdk']
  }


}