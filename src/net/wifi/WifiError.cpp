// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WifiError.hpp"
#include "Language/Language.hpp"
#include <string>
#include <string_view>

std::string
FormatWifiErrorForUser(const char *what) noexcept
{
  if (what == nullptr) {
    return {_("The operation failed.")};
  }
  const std::string_view w{what};

  if (w == std::string_view{WifiError::GONE})
    return {_("The selected network is not available. Try scanning again.")};

  if (w == std::string_view{WifiError::NEED_KEY})
    return {_("A passphrase is required, or a saved network for this "
              "name must exist in the system settings.")};

  if (w == std::string_view{WifiError::NM_FAIL} ||
      w == std::string_view{WifiError::CM_FAIL})
    return {_("The Wi-Fi connection was refused, or the passphrase is "
              "wrong.")};

  if (w == std::string_view{WifiError::NM_TIMEOUT} ||
      w == std::string_view{WifiError::CM_TIMEOUT})
    return {_("The Wi-Fi connection did not complete in time.")};

  if (w.find("org.freedesktop.") != std::string_view::npos) {
    return {_("A D-Bus error occurred. Is NetworkManager or ConnMan running "
              "on the bus?")};
  }
  if (w.find("DBus.Error") != std::string_view::npos ||
      w.find("DBus:") != std::string_view::npos) {
    return {_("A D-Bus error occurred. Is NetworkManager or ConnMan running "
              "on the bus?")};
  }
  if (w.find("Not authorized") != std::string_view::npos ||
      w.find("org.freedesktop.DBus.Error.AccessDenied") != std::string_view::npos) {
    return {_("Permission denied. You may not be allowed to connect to this "
              "network.")};
  }
  if (w.find("secret") != std::string_view::npos && w.length() < 200U) {
    return {_("A passphrase is required, or a saved network for this "
              "name must exist in the system settings.")};
  }
  if (w.length() < 200U) {
    return {what};
  }
  return {_("Could not connect. Check the passphrase and try again.")};
}
