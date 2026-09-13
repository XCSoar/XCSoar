// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemPower.hpp"

#include <exception>

#if defined(__linux__) && !defined(ANDROID)
#include "LogFile.hpp"
#ifdef KOBO
#include "Kobo/System.hpp"
#else
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/Login1.hxx"
#include "util/PrintException.hxx"
#endif
#endif

namespace SystemPower {

Capabilities
GetCapabilities() noexcept
{
#if defined(__linux__) && !defined(ANDROID)
#ifdef KOBO
  return {true, true};
#else
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
#endif

  return {};
}

bool
Reboot() noexcept
{
#if defined(__linux__) && !defined(ANDROID)
#ifdef KOBO
  return KoboReboot();
#else
  try {
    auto connection = ODBus::Connection::GetSystem();
    Login1::Reboot(connection);
    return true;
  } catch (...) {
    PrintException(std::current_exception());
  }
#endif
#endif

  return false;
}

bool
PowerOff() noexcept
{
#if defined(__linux__) && !defined(ANDROID)
#ifdef KOBO
  return KoboRequestPowerOff() || KoboPowerOff();
#else
  try {
    auto connection = ODBus::Connection::GetSystem();
    Login1::PowerOff(connection);
    return true;
  } catch (...) {
    PrintException(std::current_exception());
  }
#endif
#endif

  return false;
}

} // namespace SystemPower
