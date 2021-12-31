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

#include "ConfiguredPort.hpp"
#include "UDPPort.hpp"
#include "TCPPort.hpp"
#include "K6BtPort.hpp"
#include "Device/Config.hpp"
#include "LogFile.hpp"
#include "util/ConvertString.hpp"
#include "TCPClientPort.hpp"

#ifdef ANDROID
#include "AndroidBluetoothPort.hpp"
#include "AndroidIOIOUartPort.hpp"
#include "AndroidUsbSerialPort.hpp"
#endif

#if defined(HAVE_POSIX)
#include "TTYPort.hpp"
#else
#include "SerialPort.hpp"
#endif

#ifndef NDEBUG
#include "DumpPort.hpp"
#endif

#include <stdexcept>
#include <system_error>

#if defined(HAVE_POSIX) && !defined(ANDROID)
#include <unistd.h>
#include <errno.h>
#endif

#include <windef.h> /* for MAX_PATH */

/**
 * Attempt to detect the GPS device.
 *
 * See http://msdn.microsoft.com/en-us/library/bb202042.aspx
 */
static bool
DetectGPS(TCHAR *path, std::size_t path_max_size)
{
  return false;
}

static std::unique_ptr<Port>
WrapPort(const DeviceConfig &config, PortListener *listener,
         DataHandler &handler, std::unique_ptr<Port> port) noexcept
{
  if (config.k6bt && config.MaybeBluetooth())
    port = std::make_unique<K6BtPort>(std::move(port), config.baud_rate,
                                      listener, handler);

#ifndef NDEBUG
  if (config.dump_port)
    port = std::make_unique<DumpPort>(std::move(port));
#endif

  return port;
}

static std::unique_ptr<Port>
OpenPortInternal(EventLoop &event_loop, Cares::Channel &cares,
                 const DeviceConfig &config, PortListener *listener,
                 DataHandler &handler)
{
  const TCHAR *path = nullptr;
  TCHAR buffer[MAX_PATH];

  switch (config.port_type) {
  case DeviceConfig::PortType::DISABLED:
    throw std::runtime_error("Port is disabled");

  case DeviceConfig::PortType::SERIAL:
    if (config.path.empty())
      throw std::runtime_error("No port path configured");

    path = config.path.c_str();
    break;

  case DeviceConfig::PortType::BLE_HM10:
#ifdef ANDROID
    if (config.bluetooth_mac.empty())
      throw std::runtime_error("No Bluetooth MAC configured");

    return OpenAndroidBleHm10Port(config.bluetooth_mac, listener, handler);
#else
    throw std::runtime_error("Bluetooth not available");
#endif

  case DeviceConfig::PortType::RFCOMM:
#ifdef ANDROID
    if (config.bluetooth_mac.empty())
      throw std::runtime_error("No Bluetooth MAC configured");

    return OpenAndroidBluetoothPort(config.bluetooth_mac, listener, handler);
#else
    throw std::runtime_error("Bluetooth not available");
#endif

  case DeviceConfig::PortType::RFCOMM_SERVER:
#ifdef ANDROID
    return OpenAndroidBluetoothServerPort(listener, handler);
#else
    throw std::runtime_error("Bluetooth not available");
#endif

  case DeviceConfig::PortType::IOIOUART:
#if defined(ANDROID)
    if (config.ioio_uart_id >= AndroidIOIOUartPort::getNumberUarts())
      throw std::runtime_error("No IOIOUart configured in profile");

    return OpenAndroidIOIOUartPort(config.ioio_uart_id, config.baud_rate,
                                   listener, handler);
#else
    throw std::runtime_error("IOIO driver not available");
#endif

  case DeviceConfig::PortType::AUTO:
    if (!DetectGPS(buffer, sizeof(buffer)))
      throw std::runtime_error("No GPS detected");

    LogFormat(_T("GPS detected: %s"), buffer);

    path = buffer;
    break;

  case DeviceConfig::PortType::INTERNAL:
  case DeviceConfig::PortType::BLE_SENSOR:
  case DeviceConfig::PortType::DROIDSOAR_V2:
  case DeviceConfig::PortType::NUNCHUCK:
  case DeviceConfig::PortType::I2CPRESSURESENSOR:
  case DeviceConfig::PortType::IOIOVOLTAGE:
  case DeviceConfig::PortType::GLIDER_LINK:
    break;

  case DeviceConfig::PortType::TCP_CLIENT: {
    const WideToUTF8Converter ip_address(config.ip_address);
    if (!ip_address.IsValid())
      throw std::runtime_error("No IP address configured");

    return std::make_unique<TCPClientPort>(event_loop, cares,
                                           ip_address, config.tcp_port,
                                           listener, handler);
  }

  case DeviceConfig::PortType::TCP_LISTENER:
    return std::make_unique<TCPPort>(event_loop, config.tcp_port,
                                     listener, handler);

  case DeviceConfig::PortType::UDP_LISTENER:
    return std::make_unique<UDPPort>(event_loop, config.tcp_port,
                                     listener, handler);

  case DeviceConfig::PortType::PTY: {
#if defined(HAVE_POSIX) && !defined(ANDROID)
    if (config.path.empty())
      throw std::runtime_error("No pty path configured");

    if (unlink(config.path.c_str()) < 0 && errno != ENOENT)
      throw std::system_error(std::error_code(errno,
                                              std::system_category()),
                              "Failed to delete pty");

    auto port = std::make_unique<TTYPort>(event_loop, listener, handler);
    const char *slave_path = port->OpenPseudo();

    if (symlink(slave_path, config.path.c_str()) < 0)
      throw std::system_error(std::error_code(errno,
                                              std::system_category()),
                              "Failed to symlink pty");

    return port;
#else
    throw std::runtime_error("Pty not available");
#endif
  }

  case DeviceConfig::PortType::ANDROID_USB_SERIAL:
#ifdef ANDROID
    if (config.path.empty())
      throw std::runtime_error("No name configured");

    return OpenAndroidUsbSerialPort(config.path.c_str(), config.baud_rate,
                                    listener, handler);
#else
    throw std::runtime_error("Android USB serial not available");
#endif
  }

  if (path == nullptr)
    throw std::runtime_error("No port path configured");

#ifdef HAVE_POSIX
  auto port = std::make_unique<TTYPort>(event_loop, listener, handler);
#else
  auto port = std::make_unique<SerialPort>(listener, handler);
#endif
  port->Open(path, config.baud_rate);
  return port;
}

std::unique_ptr<Port>
OpenPort(EventLoop &event_loop, Cares::Channel &cares,
         const DeviceConfig &config, PortListener *listener,
         DataHandler &handler)
{
  auto port = OpenPortInternal(event_loop, cares, config, listener, handler);
  if (port != nullptr)
    port = WrapPort(config, listener, handler, std::move(port));
  return port;
}
