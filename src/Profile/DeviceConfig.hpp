/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_PROFILE_DEVICE_CONFIG_HPP
#define XCSOAR_PROFILE_DEVICE_CONFIG_HPP

#include "Util/StaticString.hpp"

#include <tchar.h>

/**
 * Configuration structure for serial devices
 */
struct DeviceConfig {
  enum port_type {
    /**
     * This device is disabled.
     */
    DISABLED,

    /**
     * Serial port, i.e. COMx / RS-232.
     */
    SERIAL,

    /**
     * Bluetooth RFCOMM to a paired device.
     */
    RFCOMM,

    /**
     * Attempt to auto-discover the GPS source.
     *
     * On Windows CE, this opens the GPS Intermediate Driver Multiplexer.
     * @see http://msdn.microsoft.com/en-us/library/bb202042.aspx
     */
    AUTO,

    /**
     * The built-in GPS receiver.
     */
    INTERNAL,

    /**
     * Listen on a TCP port.
     */
    TCP_LISTENER,
  };

  port_type port_type;          /**< Type of the port */

  /**
   * The baud rate of the device in NMEA mode.
   */
  unsigned baud_rate;

  /**
   * The path name of the serial port, e.g. "COM4:" or "/dev/ttyUSB0".
   */
  StaticString<64> path;

  /**
   * The Bluetooth MAC address of the peer.
   */
  StaticString<32> bluetooth_mac;

  /**
   * Name of the driver.
   */
  StaticString<32> driver_name;

  /**
   * Does this port type use a baud rate?
   */
  static bool UsesSpeed(enum port_type port_type) {
    return port_type == SERIAL || port_type == AUTO;
  }

  bool UsesSpeed() const {
    return UsesSpeed(port_type);
  }

  /**
   * Does this port type use a driver?
   */
  static bool UsesDriver(enum port_type port_type) {
    return port_type == SERIAL || port_type == RFCOMM || port_type == AUTO ||
      port_type == TCP_LISTENER;
  }

  bool UsesDriver() const {
    return UsesDriver(port_type);
  }

  bool IsDriver(const TCHAR *name) const {
    return UsesDriver() && driver_name.equals(name);
  }

  bool IsVega() const {
    return IsDriver(_T("Vega"));
  }
};

namespace Profile
{
  void GetDeviceConfig(unsigned n, DeviceConfig &config);
  void SetDeviceConfig(unsigned n, const DeviceConfig &config);
};

#endif
