/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "util/StaticString.hxx"

#include <cstdint>

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
     * Bluetooth Low Energy sensor.
     */
    BLE_SENSOR,

    /**
     * Bluetooth Low Energy HM10 protocol to a paired device.  Unlike
     * #BLE_SENSOR, this provides a bidirectional data stream and
     * XCSoar accesses it through the #Port interface.
     */
    BLE_HM10,

    /**
     * A GliderLink broadcast receiver. Available on Android only
     */
    GLIDER_LINK,

    /**
     * USB serial port on Android.
     */
    ANDROID_USB_SERIAL,
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
  static constexpr bool UsesSpeed(PortType port_type) noexcept {
    return port_type == PortType::SERIAL || port_type == PortType::AUTO ||
      port_type == PortType::ANDROID_USB_SERIAL ||
      port_type == PortType::IOIOUART;
  }

  static constexpr bool UsesBluetoothMac(PortType port_type) noexcept {
    return port_type == PortType::RFCOMM ||
      port_type == PortType::BLE_SENSOR ||
      port_type == PortType::BLE_HM10;
  }

  constexpr bool IsDisabled() const noexcept {
    return !enabled || port_type == PortType::DISABLED;
  }

  /**
   * Checks if the specified DeviceConfig is available on this platform.
   */
  [[gnu::pure]]
  bool IsAvailable() const noexcept;

  /**
   * Should this device be reopened when no data has been received for
   * a certain amount of time?  Some ports need this to recover from
   * errors.
   */
  [[gnu::pure]]
  bool ShouldReopenOnTimeout() const noexcept;

  [[gnu::pure]]
  static bool MaybeBluetooth(PortType port_type, const TCHAR *path) noexcept;

  [[gnu::pure]]
  bool MaybeBluetooth() const noexcept;

  /**
   * Check whether the Bluetooth device name starts with the specified
   * prefix.  Returns false on mismatch or if the name could not be
   * determined or if this is not a Bluetooth device.
   */
  [[gnu::pure]]
  bool BluetoothNameStartsWith(const char *prefix) const noexcept;

  bool UsesSpeed() const noexcept {
    return UsesSpeed(port_type) ||
      (MaybeBluetooth() && k6bt);
  }

  bool UsesBluetoothMac() const noexcept {
    return UsesBluetoothMac(port_type);
  }

  /**
   * Does this port type use a driver?
   */
  static constexpr bool UsesDriver(PortType port_type) noexcept {
    switch (port_type) {
    case PortType::DISABLED:
    case PortType::BLE_SENSOR:
    case PortType::GLIDER_LINK:
    case PortType::DROIDSOAR_V2:
    case PortType::NUNCHUCK:
    case PortType::I2CPRESSURESENSOR:
    case PortType::IOIOVOLTAGE:
    case PortType::INTERNAL:
      return false;

    case PortType::SERIAL:
    case PortType::BLE_HM10:
    case PortType::RFCOMM:
    case PortType::RFCOMM_SERVER:
    case PortType::AUTO:
    case PortType::TCP_LISTENER:
    case PortType::TCP_CLIENT:
    case PortType::IOIOUART:
    case PortType::PTY:
    case PortType::UDP_LISTENER:
    case PortType::ANDROID_USB_SERIAL:
      return true;
    }

    /* unreachable */
    return false;
  }

  constexpr bool UsesDriver() const noexcept {
    return UsesDriver(port_type);
  }

  /**
   * Does this port type use a tcp host?
   */
  static constexpr bool UsesIPAddress(PortType port_type) noexcept {
    return port_type == PortType::TCP_CLIENT;
  }

  constexpr bool UsesIPAddress() const noexcept {
    return UsesIPAddress(port_type);
  }

  /**
   * Does this port type use a tcp port?
   */
  static constexpr bool UsesTCPPort(PortType port_type) noexcept {
    return port_type == PortType::TCP_LISTENER ||
      port_type == PortType::TCP_CLIENT ||
      port_type == PortType::UDP_LISTENER;
  }

  constexpr bool UsesTCPPort() const noexcept {
    return UsesTCPPort(port_type);
  }

  constexpr bool IsDriver(const TCHAR *name) const noexcept {
    return UsesDriver() && driver_name.equals(name);
  }

  bool IsVega() const noexcept {
    return IsDriver(_T("Vega"));
  }

  constexpr bool IsAndroidInternalGPS() const noexcept {
#ifdef ANDROID
    return port_type == PortType::INTERNAL;
#else
    return false;
#endif
  }

  static constexpr bool UsesPort(PortType port_type) noexcept {
    return UsesDriver(port_type);
  }

  constexpr bool UsesPort() const noexcept {
    return UsesPort(port_type);
  }

  static constexpr bool IsPressureSensor(PortType port_type) noexcept {
    return port_type == PortType::I2CPRESSURESENSOR;
  }

  constexpr bool IsPressureSensor() const noexcept {
    return IsPressureSensor(port_type);
  }

  static constexpr bool UsesI2C(PortType port_type) noexcept {
    return port_type == PortType::NUNCHUCK ||
           port_type == PortType::I2CPRESSURESENSOR;
  }

  constexpr bool UsesI2C() const noexcept {
    return UsesI2C(port_type);
  }

  static constexpr bool UsesCalibration(PortType port_type) noexcept {
    return port_type == PortType::I2CPRESSURESENSOR ||
           port_type == PortType::DROIDSOAR_V2;
  }

  constexpr bool UsesCalibration() const noexcept {
    return UsesCalibration(port_type);
  }

  void Clear() noexcept;

  /**
   * Generates a human-readable (localised) port name.
   */
  [[gnu::pure]]
  const TCHAR *GetPortName(TCHAR *buffer, size_t max_size) const noexcept;
};

#endif
