// SPDX-License-Identifier: BSD-2-Clause
// Copyright The XCSoar Project

#pragma once

namespace ODBus {
class Connection;
}

/**
 * Wrappers for org.freedesktop.login1.Manager.
 */
namespace Login1 {

/**
 * Returns whether the action is available without authentication.
 * Throws on D-Bus errors.
 */
bool
CanReboot(ODBus::Connection &connection);

/**
 * Returns whether the action is available without authentication.
 * Throws on D-Bus errors.
 */
bool
CanPowerOff(ODBus::Connection &connection);

/**
 * Requests an orderly system reboot without interactive authentication.
 * Throws on D-Bus errors.
 */
void
Reboot(ODBus::Connection &connection);

/**
 * Requests an orderly system power-off without interactive authentication.
 * Throws on D-Bus errors.
 */
void
PowerOff(ODBus::Connection &connection);

} // namespace Login1
