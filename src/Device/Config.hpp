/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_DEVICE_CONFIG_HPP
#define XCSOAR_DEVICE_CONFIG_HPP

#include "Util/StaticString.hxx"

#include <stdint.h>
#include <tchar.h>

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
     * Listen for incoming Bluetooth RFCOMM connections.
     */
    RFCOMM_SERVER,

    /**
     * Android IOIO UArt device
     */
    IOIOUART,

    /**
     * Android IOIO Misc. devices
     */
    DROIDSOAR_V2,
    NUNCHUCK,
    I2CPRESSURESENSOR,
    IOIOVOLTAGE,

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
     * Connect to a TCP port.
     */
    TCP_CLIENT,

    /**
     * Listen on a TCP Network port.
     */
    TCP_LISTENER,

    /**
     * Listen on a UDP Network port.
     */
    UDP_LISTENER,

    /**
     * A master pseudo-terminal.  The "path" attribute specifies the
     * path of a symlink pointing to the slave pseudo-terminal.  Only
     * for debugging.
     */
    PTY,

    /**
     * A GliderLink broadcast receiver. Available on Android only
     */
    GLIDER_LINK,
  };

  /**
   * Type of the port
   */
  PortType port_type;

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
   * The IOIO I2C bus.
   */
  /* For some devices (e.g DroidSoar) this may hold the i2c address # in bits 15-8
   * and other device specific data in 31-16.
   * In these cases i2c_addr is not used. */
  unsigned i2c_bus;
  unsigned i2c_addr;

  /**
   * What is the purpose of this pressure sensor.
   */
  enum class PressureUse : unsigned {
    NONE = 0,
    /** ProvidePressureAltitude() and ProvideNoncompVario() */
    STATIC_WITH_VARIO,
    /** ProvidePressureAltitude() */
    STATIC_ONLY,
    /** ProvideNettoVario() */
    TEK_PRESSURE,
    /** ProvideIndicatedAirspeedWithAltitude() */
    PITOT,
    /** Determine and then save the offset between static and pitot sensor */
    PITOT_ZERO,
  } press_use;

  /**
   * sensor calibration data
   */
  double sensor_offset;
  double sensor_factor;

  /**
   * Name of the driver.
   */
  StaticString<32> driver_name;

  /**
   * useflag and Name of the second driver for passed through device.
   */
  bool use_second_device;
  StaticString<32> driver2_name;

  /**
   * The IP address of the peer to connect to, including the port
   * number.  Used for #TCP_CLIENT.
   */
  StaticString<64> ip_address;

  /**
   * The TCP server port number.  Used for #TCP_LISTENER and
   * #TCP_CLIENT.  Also used for #UDP_LISTENER (which is a kludge
   * because there's no #udp_port attribute).
   */
  unsigned tcp_port;

  /**
   * Is this device currently enabled?  This flag can be used to
   * maintain a stock of devices, and not all of them are enabled at a
   * time.  For example, you can disable the logger connection while
   * you're flying and re-enable it for downloading the flight.
   */
  bool enabled;

  /**
   * Use the K6-Bt protocol?
   */
  bool k6bt;

#ifndef NDEBUG
  /**
   * Use the DumpPort wrapper for debugging
   */
  bool dump_port;
#endif

  /**
   * Should XCSoar send MC value, bug, ballast, etc. to the device
   */
  bool sync_to_device;

  /**
   * Should XCSoar use the MC value, bug, ballast, etc. received from the device
   */
  bool sync_from_device;

  /**
   * Does this port type use a baud rate?
   */
  static bool UsesSpeed(PortType port_type) {
    return port_type == PortType::SERIAL || port_type == PortType::AUTO ||
      port_type == PortType::IOIOUART;
  }

  bool IsDisabled() const {
    return !enabled || port_type == PortType::DISABLED;
  }

  /**
   * Checks if the specified DeviceConfig is available on this platform.
   */
  gcc_pure
  bool IsAvailable() const;

  /**
   * Should this device be reopened when no data has been received for
   * a certain amount of time?  Some ports need this to recover from
   * errors.
   */
  gcc_pure
  bool ShouldReopenOnTimeout() const;

  gcc_pure
  static bool MaybeBluetooth(PortType port_type, const TCHAR *path);

  gcc_pure
  bool MaybeBluetooth() const;

  /**
   * Check whether the Bluetooth device name starts with the specified
   * prefix.  Returns false on mismatch or if the name could not be
   * determined or if this is not a Bluetooth device.
   */
  gcc_pure
  bool BluetoothNameStartsWith(const char *prefix) const;

  bool UsesSpeed() const {
    return UsesSpeed(port_type) ||
      (MaybeBluetooth() && k6bt);
  }

  /**
   * Does this port type use a driver?
   */
  static bool UsesDriver(PortType port_type) {
    return port_type == PortType::SERIAL || port_type == PortType::RFCOMM ||
      port_type == PortType::RFCOMM_SERVER ||
      port_type == PortType::AUTO || port_type == PortType::TCP_LISTENER ||
      port_type == PortType::TCP_CLIENT ||
      port_type == PortType::IOIOUART || port_type == PortType::PTY ||
      port_type == PortType::UDP_LISTENER;
  }

  bool UsesDriver() const {
    return UsesDriver(port_type);
  }

  /**
   * Does this port type use a tcp host?
   */
  static bool UsesIPAddress(PortType port_type) {
    return port_type == PortType::TCP_CLIENT;
  }

  bool UsesIPAddress() const {
    return UsesIPAddress(port_type);
  }

  /**
   * Does this port type use a tcp port?
   */
  static bool UsesTCPPort(PortType port_type) {
    return port_type == PortType::TCP_LISTENER ||
      port_type == PortType::TCP_CLIENT ||
      port_type == PortType::UDP_LISTENER;
  }

  bool UsesTCPPort() const {
    return UsesTCPPort(port_type);
  }

  bool IsDriver(const TCHAR *name) const {
    return UsesDriver() && driver_name.equals(name);
  }

  bool IsVega() const {
    return IsDriver(_T("Vega"));
  }

  bool IsAndroidInternalGPS() const {
#ifdef ANDROID
    return port_type == PortType::INTERNAL;
#else
    return false;
#endif
  }

  static bool UsesPort(PortType port_type) {
    return UsesDriver(port_type);
  }

  bool UsesPort() const {
    return UsesPort(port_type);
  }

  static bool IsPressureSensor(PortType port_type) {
    return port_type == PortType::I2CPRESSURESENSOR;
  }

  bool IsPressureSensor() const {
    return IsPressureSensor(port_type);
  }

  static bool UsesI2C(PortType port_type) {
    return port_type == PortType::NUNCHUCK ||
           port_type == PortType::I2CPRESSURESENSOR;
  }

  bool UsesI2C() const {
    return UsesI2C(port_type);
  }

  static bool UsesCalibration(PortType port_type) {
    return port_type == PortType::I2CPRESSURESENSOR ||
           port_type == PortType::DROIDSOAR_V2;
  }

  bool UsesCalibration() const {
    return UsesCalibration(port_type);
  }

  void Clear();

  /**
   * Generates a human-readable (localised) port name.
   */
  gcc_pure
  const TCHAR *GetPortName(TCHAR *buffer, size_t max_size) const;
};

#endif
