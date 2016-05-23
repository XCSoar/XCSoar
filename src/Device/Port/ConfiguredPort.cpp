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

#include "ConfiguredPort.hpp"
#include "SocketPort.hpp"
#include "TCPPort.hpp"
#include "K6BtPort.hpp"
#include "Device/Config.hpp"
#include "LogFile.hpp"
#include "Util/ConvertString.hpp"
#include "TCPClientPort.hpp"

#ifdef ANDROID
#include "AndroidBluetoothPort.hpp"
#include "AndroidIOIOUartPort.hpp"
#endif

#if defined(HAVE_POSIX)
#include "TTYPort.hpp"
#else
#include "SerialPort.hpp"
#endif

#ifndef NDEBUG
#include "DumpPort.hpp"
#endif

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
DetectGPS(TCHAR *path, size_t path_max_size)
{
  return false;
}

static Port *
WrapPort(const DeviceConfig &config, PortListener *listener,
         DataHandler &handler, Port *port)
{
  if (config.k6bt && config.MaybeBluetooth())
    port = new K6BtPort(port, config.baud_rate, listener, handler);

#ifndef NDEBUG
  if (config.dump_port)
    port = new DumpPort(port);
#endif

  return port;
}

static Port *
OpenPortInternal(boost::asio::io_service &io_service,
                 const DeviceConfig &config, PortListener *listener,
                 DataHandler &handler)
{
  const TCHAR *path = nullptr;
  TCHAR buffer[MAX_PATH];

  switch (config.port_type) {
  case DeviceConfig::PortType::DISABLED:
    return nullptr;

  case DeviceConfig::PortType::SERIAL:
    if (config.path.empty())
      return nullptr;

    path = config.path.c_str();
    break;

  case DeviceConfig::PortType::RFCOMM:
#ifdef ANDROID
    if (config.bluetooth_mac.empty()) {
      LogFormat("No Bluetooth MAC configured");
      return nullptr;
    }

    return OpenAndroidBluetoothPort(config.bluetooth_mac, listener, handler);
#else
    LogFormat("Bluetooth not available on this platform");
    return nullptr;
#endif

  case DeviceConfig::PortType::RFCOMM_SERVER:
#ifdef ANDROID
    return OpenAndroidBluetoothServerPort(listener, handler);
#else
    LogFormat("Bluetooth not available on this platform");
    return nullptr;
#endif

  case DeviceConfig::PortType::IOIOUART:
#if defined(ANDROID)
    if (config.ioio_uart_id >= AndroidIOIOUartPort::getNumberUarts()) {
      LogFormat("No IOIOUart configured in profile");
      return nullptr;
    }

    return OpenAndroidIOIOUartPort(config.ioio_uart_id, config.baud_rate,
                                   listener, handler);
#else
    LogFormat("IOIO Uart not available on this platform or version");
    return nullptr;
#endif

  case DeviceConfig::PortType::AUTO:
    if (!DetectGPS(buffer, sizeof(buffer))) {
      LogFormat("no GPS detected");
      return nullptr;
    }

    LogFormat(_T("GPS detected: %s"), buffer);

    path = buffer;
    break;

  case DeviceConfig::PortType::INTERNAL:
  case DeviceConfig::PortType::DROIDSOAR_V2:
  case DeviceConfig::PortType::NUNCHUCK:
  case DeviceConfig::PortType::I2CPRESSURESENSOR:
  case DeviceConfig::PortType::IOIOVOLTAGE:
    break;

  case DeviceConfig::PortType::TCP_CLIENT: {
    const WideToUTF8Converter ip_address(config.ip_address);
    if (!ip_address.IsValid())
      return nullptr;

    auto port = new TCPClientPort(io_service, listener, handler);
    if (!port->Connect(ip_address, config.tcp_port)) {
      delete port;
      return nullptr;
    }

    return port;
  }

  case DeviceConfig::PortType::TCP_LISTENER: {
    TCPPort *port = new TCPPort(io_service, listener, handler);
    if (!port->Open(config.tcp_port)) {
      delete port;
      return nullptr;
    }

    return port;
  }

  case DeviceConfig::PortType::UDP_LISTENER: {
    SocketPort *port = new SocketPort(listener, handler);
    if (!port->OpenUDPListener(config.tcp_port)) {
      delete port;
      return nullptr;
    }

    return port;
  }

  case DeviceConfig::PortType::PTY: {
#if defined(HAVE_POSIX) && !defined(ANDROID)
    if (config.path.empty())
      return nullptr;

    if (unlink(config.path.c_str()) < 0 && errno != ENOENT)
      return nullptr;

    TTYPort *port = new TTYPort(io_service, listener, handler);
    const char *slave_path = port->OpenPseudo();
    if (slave_path == nullptr) {
      delete port;
      return nullptr;
    }

    if (symlink(slave_path, config.path.c_str()) < 0) {
      delete port;
      return nullptr;
    }

    return port;
#else
    return nullptr;
#endif
  }
  }

  if (path == nullptr)
    return nullptr;

#ifdef HAVE_POSIX
  TTYPort *port = new TTYPort(io_service, listener, handler);
#else
  SerialPort *port = new SerialPort(listener, handler);
#endif
  if (!port->Open(path, config.baud_rate)) {
    delete port;
    return nullptr;
  }

  return port;
}

Port *
OpenPort(boost::asio::io_service &io_service,
         const DeviceConfig &config, PortListener *listener,
         DataHandler &handler)
{
  Port *port = OpenPortInternal(io_service, config, listener, handler);
  if (port != nullptr)
    port = WrapPort(config, listener, handler, port);
  return port;
}
