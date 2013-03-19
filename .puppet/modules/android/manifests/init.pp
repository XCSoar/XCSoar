# == Class: android
#
# This class is used to install the android SDK and platform tools
#
# === Parameters:
#
# [*version*] the SDK version
# [*user*] used to set the file ownership of the installed SDK
# [*group*] used to set the file ownership of the installed SDK
# [*installdir*] the install directory.
# [*proxy_host*] the proxy server host name (used by the android tool)
# [*proxy_port*] the proxy server port (used by the android tool)
#
# === Authors
#
# Etienne Pelletier <epelletier@maestrodev.com>
#
# === Copyright
#
# Copyright 2012 MaestroDev, unless otherwise noted.
#
class android(
  $version    = $android::params::version,
  $ndk_version    = $android::params::ndk_version,
  $user       = $android::params::user,
  $group      = $android::params::group,
  $installdir = $android::params::installdir,
  $proxy_host = $android::params::proxy_host,
  $proxy_port = $android::params::proxy_port) inherits android::params {

  include android::paths
  include android::sdk
  include android::ndk
  include android::platform_tools
  include android::tools
}