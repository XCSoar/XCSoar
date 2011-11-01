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
#include <stdint.h>

/**
 * Configuration structure for serial devices
 */
struct DeviceConfig {
  enum class PortType : uint8_t {
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
     * Android IOIO UArt device
     */
    IOIOUART,

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

  PortType port_type;          /**< Type of the port */

  /**
   * The baud rate of the device in NMEA mode.
   */
  unsigned baud_rate;

  /**
   * The baud rate of the device for bulk transfer (e.g. task
   * declaration, flight download).  Not used by all drivers, see
   * Driver::SupportsBulkBaudRate().
   *
   * The special value "0" means same value as #baud_rate.
   */
  unsigned bulk_baud_rate;

  /**
   * The path name of the serial port, e.g. "COM4:" or "/dev/ttyUSB0".
   */
  StaticString<64> path;

  /**
   * The Bluetooth MAC address of the peer.
   */
  StaticString<32> bluetooth_mac;

  /**
   * The IOIO UART ID.
   */
  unsigned ioio_uart_id;

  /**
   * Name of the driver.
   */
  StaticString<32> driver_name;

  /**
   * Does this port type use a baud rate?
   */
  static bool UsesSpeed(PortType port_type) {
    return port_type == PortType::SERIAL || port_type == PortType::AUTO ||
      port_type == PortType::IOIOUART;
  }

  /**
   * Checks if the specified DeviceConfig is available on this
   * platform.
   */
  gcc_pure
  bool IsAvailable() const;

  bool UsesSpeed() const {
    return UsesSpeed(port_type);
  }

  /**
   * Does this port type use a driver?
   */
  static bool UsesDriver(PortType port_type) {
    return port_type == PortType::SERIAL || port_type == PortType::RFCOMM ||
      port_type == PortType::AUTO || port_type == PortType::TCP_LISTENER ||
      port_type == PortType::IOIOUART;
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

  bool UsesPort() const {
    return UsesDriver();
  }

  void Clear() {
    port_type = PortType::DISABLED;
    baud_rate = 4800u;
    path.clear();
    bluetooth_mac.clear();
    driver_name.clear();
  }

  /**
   * Generates a human-readable (localised) port name.
   */
  gcc_pure
  const TCHAR *GetPortName(TCHAR *buffer, size_t max_size) const;
};

namespace Profile
{
  void GetDeviceConfig(unsigned n, DeviceConfig &config);
  void SetDeviceConfig(unsigned n, const DeviceConfig &config);
};

#endif
