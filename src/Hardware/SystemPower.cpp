// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemPower.hpp"

#include <exception>

#if defined(__linux__) && !defined(ANDROID) && !defined(KOBO)
#include "LogFile.hpp"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Login1.hxx"
#include "util/PrintException.hxx"
#endif

namespace SystemPower {

Capabilities
GetCapabilities() noexcept
{
#if defined(__linux__) && !defined(ANDROID) && !defined(KOBO)
  try {
    auto connection = ODBus::Connection::GetSystem();
    return {
      Login1::CanReboot(connection),
      Login1::CanPowerOff(connection),
    };
  } catch (...) {
    LogError(std::current_exception(),
             "Failed to query system power capabilities");
  }
#endif

  return {};
}

bool
Reboot() noexcept
{
#if defined(__linux__) && !defined(ANDROID) && !defined(KOBO)
  try {
    auto connection = ODBus::Connection::GetSystem();
    Login1::Reboot(connection);
    return true;
  } catch (...) {
    PrintException(std::current_exception());
  }
#endif

  return false;
}

bool
PowerOff() noexcept
{
#if defined(__linux__) && !defined(ANDROID) && !defined(KOBO)
  try {
    auto connection = ODBus::Connection::GetSystem();
    Login1::PowerOff(connection);
    return true;
  } catch (...) {
    PrintException(std::current_exception());
  }
#endif

  return false;
}

} // namespace SystemPower
