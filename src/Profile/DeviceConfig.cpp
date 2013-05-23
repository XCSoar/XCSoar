/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Profile/DeviceConfig.hpp"
#include "Profile/Profile.hpp"
#include "Asset.hpp"
#include "Language/Language.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#include "Java/Global.hpp"
#endif

#include <stdio.h>

static const char *const port_type_strings[] = {
  "disabled",
  "serial",
  "rfcomm",
  "rfcomm_server",
  "ioio_uart",
  "droidsoar_v2",
  "nunchuck",
  "i2c_baro",
  "ioio_voltage",
  "auto",
  "internal",
  "tcp_listener",
  "udp_listener",
  "pty",
  NULL
};

bool
DeviceConfig::IsAvailable() const
{
  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
    return true;

  case PortType::RFCOMM:
  case PortType::RFCOMM_SERVER:
    return IsAndroid();

  case PortType::IOIOUART:
  case PortType::DROIDSOAR_V2:
  case PortType::NUNCHUCK:
  case PortType::I2CPRESSURESENSOR:
  case PortType::IOIOVOLTAGE:
    return HasIOIOLib();

  case PortType::AUTO:
    return IsWindowsCE();

  case PortType::INTERNAL:
    return IsAndroid();

  case PortType::TCP_LISTENER:
  case PortType::UDP_LISTENER:
    return true;

  case PortType::PTY:
#if defined(HAVE_POSIX) && !defined(ANDROID)
    return true;
#else
    return false;
#endif
  }

  /* unreachable */
  return false;
}

bool
DeviceConfig::ShouldReopenOnTimeout() const
{
  switch (port_type) {
  case PortType::DISABLED:
    return false;

  case PortType::SERIAL:
  case PortType::AUTO:
    /* auto-reopen on Windows CE due to its quirks, but not Altair,
       because Altair ports are known to be "kind of sane" (no flaky
       Bluetooth drivers, because there's no Bluetooth) */
    return IsWindowsCE() && !IsAltair();

  case PortType::RFCOMM:
  case PortType::RFCOMM_SERVER:
  case PortType::IOIOUART:
  case PortType::DROIDSOAR_V2:
  case PortType::NUNCHUCK:
  case PortType::I2CPRESSURESENSOR:
  case PortType::IOIOVOLTAGE:
    /* errors on these are detected automatically by the driver */
    return false;

  case PortType::INTERNAL:
    /* reopening the Android internal GPS doesn't help */
    return false;

  case PortType::TCP_LISTENER:
  case PortType::UDP_LISTENER:
    /* this is a server, and if no data gets received, this can just
       mean that nobody connected to it, but reopening it periodically
       doesn't help */
    return false;

  case PortType::PTY:
    return false;
  }

  gcc_unreachable();
}

bool
DeviceConfig::BluetoothNameStartsWith(const char *prefix) const
{
#ifdef ANDROID
  if (port_type != PortType::RFCOMM)
    return false;

  const char *name =
    BluetoothHelper::GetNameFromAddress(Java::GetEnv(), bluetooth_mac.c_str());
  return name != nullptr && StringStartsWith(name, prefix);
#else
  return false;
#endif
}

const TCHAR *
DeviceConfig::GetPortName(TCHAR *buffer, size_t max_size) const
{
  switch (port_type) {
  case PortType::DISABLED:
    return _("Disabled");

  case PortType::SERIAL:
    return path.c_str();

  case PortType::RFCOMM: {
    const TCHAR *name = bluetooth_mac.c_str();
#ifdef ANDROID
    const char *name2 =
      BluetoothHelper::GetNameFromAddress(Java::GetEnv(), name);
    if (name2 != nullptr)
      name = name2;
#endif

    StringFormat(buffer, max_size, _T("Bluetooth %s"), name);
    return buffer;
    }

  case PortType::RFCOMM_SERVER:
    return _("Bluetooth server");

  case PortType::IOIOUART:
    StringFormat(buffer, max_size, _T("IOIO UART %d"), ioio_uart_id);
    return buffer;

  case PortType::DROIDSOAR_V2:
    return _T("DroidSoar V2");

  case PortType::NUNCHUCK:
    return _T("Nunchuck");

  case PortType::I2CPRESSURESENSOR:
    return _T("IOIO i2c pressure sensor");

  case PortType::IOIOVOLTAGE:
    return _T("IOIO voltage sensor");

  case PortType::AUTO:
    return _("GPS Intermediate Driver");

  case PortType::INTERNAL:
    return _("Built-in GPS & sensors");

  case PortType::TCP_LISTENER:
    StringFormat(buffer, max_size, _T("TCP port %d"), tcp_port);
    return buffer;

  case PortType::UDP_LISTENER:
    StringFormat(buffer, max_size, _T("UDP port %d"), tcp_port);
    return buffer;

  case PortType::PTY:
    StringFormat(buffer, max_size, _T("Pseudo-terminal %s"), path.c_str());
    return buffer;
  }

  gcc_unreachable();
}

static const char *
MakeDeviceSettingName(char *buffer, const char *prefix, unsigned n,
                      const char *suffix)
{
  strcpy(buffer, prefix);

  if (n > 0)
    sprintf(buffer + strlen(buffer), "%u", n + 1);

  strcat(buffer, suffix);

  return buffer;
}

static bool
StringToPortType(const char *value, DeviceConfig::PortType &type)
{
  for (auto i = port_type_strings; *i != NULL; ++i) {
    if (StringIsEqual(value, *i)) {
      type = (DeviceConfig::PortType)std::distance(port_type_strings, i);
      return true;
    }
  }

  return false;
}

static bool
ReadPortType(unsigned n, DeviceConfig::PortType &type)
{
  char name[64];

  MakeDeviceSettingName(name, "Port", n, "Type");

  const char *value = Profile::Get(name);
  return value != NULL && StringToPortType(value, type);
}

static bool
LoadPath(DeviceConfig &config, unsigned n)
{
  char buffer[64];
  MakeDeviceSettingName(buffer, "Port", n, "Path");
  return Profile::Get(buffer, config.path);
}

static bool
LoadPortIndex(DeviceConfig &config, unsigned n)
{
  char buffer[64];
  MakeDeviceSettingName(buffer, "Port", n, "Index");

  unsigned index;
  if (!Profile::Get(buffer, index))
    return false;

  /* adjust the number, compatibility quirk for XCSoar 5 */
  if (index < 10)
    ++index;
  else if (index == 10)
    index = 0;

  TCHAR path[64];
  _stprintf(path, _T("COM%u:"), index);
  config.path = path;
  return true;
}

void
Profile::GetDeviceConfig(unsigned n, DeviceConfig &config)
{
  char buffer[64];

  bool have_port_type = ReadPortType(n, config.port_type);

  MakeDeviceSettingName(buffer, "Port", n, "BluetoothMAC");
  Get(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, "Port", n, "IOIOUartID");
  Get(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, "Port", n, "TCPPort");
  if (!Get(buffer, config.tcp_port))
    config.tcp_port = 4353;

  config.path.clear();
  if ((!have_port_type ||
       config.port_type == DeviceConfig::PortType::SERIAL) &&
      !LoadPath(config, n) && LoadPortIndex(config, n))
    config.port_type = DeviceConfig::PortType::SERIAL;

  MakeDeviceSettingName(buffer, "Port", n, "BaudRate");
  if (!Get(buffer, config.baud_rate)) {
    /* XCSoar before 6.2 used to store a "speed index", not the real
       baud rate - try to import the old settings */

    static constexpr unsigned speed_index_table[] = {
      1200,
      2400,
      4800,
      9600,
      19200,
      38400,
      57600,
      115200
    };

    MakeDeviceSettingName(buffer, "Speed", n, "Index");
    unsigned speed_index;
    if (Get(buffer, speed_index) &&
        speed_index < ARRAY_SIZE(speed_index_table))
      config.baud_rate = speed_index_table[speed_index];
  }

  MakeDeviceSettingName(buffer, "Port", n, "BulkBaudRate");
  if (!Get(buffer, config.bulk_baud_rate))
    config.bulk_baud_rate = 0;

  strcpy(buffer, "DeviceA");
  buffer[strlen(buffer) - 1] += n;
  Get(buffer, config.driver_name);

  MakeDeviceSettingName(buffer, "Port", n, "SyncFromDevice");
  Get(buffer, config.sync_from_device);

  MakeDeviceSettingName(buffer, "Port", n, "SyncToDevice");
  Get(buffer, config.sync_to_device);

  MakeDeviceSettingName(buffer, "Port", n, "K6Bt");
  Get(buffer, config.k6bt);

#ifndef NDEBUG
  MakeDeviceSettingName(buffer, "Port", n, "DumpPort");
  Get(buffer, config.dump_port);
#endif

  MakeDeviceSettingName(buffer, "Port", n, "IgnoreChecksum");
  if (!Get(buffer, config.ignore_checksum))
    Get(ProfileKeys::IgnoreNMEAChecksum, config.ignore_checksum);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Bus");
  Get(buffer, config.i2c_bus);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Addr");
  Get(buffer, config.i2c_addr);

  MakeDeviceSettingName(buffer, "Port", n, "PressureUse");
  GetEnum(buffer, config.press_use);

  MakeDeviceSettingName(buffer, "Port", n, "SensorOffset");
  Get(buffer, config.sensor_offset);

  MakeDeviceSettingName(buffer, "Port", n, "SensorFactor");
  Get(buffer, config.sensor_factor);
}

static const char *
PortTypeToString(DeviceConfig::PortType type)
{
  const unsigned i = (unsigned)type;
  return i < ARRAY_SIZE(port_type_strings)
    ? port_type_strings[i]
    : NULL;
}

static void
WritePortType(unsigned n, DeviceConfig::PortType type)
{
  const char *value = PortTypeToString(type);
  if (value == NULL)
    return;

  char name[64];
  MakeDeviceSettingName(name, "Port", n, "Type");
  Profile::Set(name, value);
}

void
Profile::SetDeviceConfig(unsigned n, const DeviceConfig &config)
{
  char buffer[64];

  WritePortType(n, config.port_type);

  MakeDeviceSettingName(buffer, "Port", n, "BluetoothMAC");
  Set(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, "Port", n, "IOIOUartID");
  Set(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, "Port", n, "Path");
  Set(buffer, config.path);

  MakeDeviceSettingName(buffer, "Port", n, "BaudRate");
  Set(buffer, config.baud_rate);

  MakeDeviceSettingName(buffer, "Port", n, "BulkBaudRate");
  Set(buffer, config.bulk_baud_rate);

  MakeDeviceSettingName(buffer, "Port", n, "TCPPort");
  Set(buffer, config.tcp_port);

  strcpy(buffer, "DeviceA");
  buffer[strlen(buffer) - 1] += n;
  Set(buffer, config.driver_name);

  MakeDeviceSettingName(buffer, "Port", n, "SyncFromDevice");
  Set(buffer, config.sync_from_device);

  MakeDeviceSettingName(buffer, "Port", n, "SyncToDevice");
  Set(buffer, config.sync_to_device);

  MakeDeviceSettingName(buffer, "Port", n, "K6Bt");
  Set(buffer, config.k6bt);

  MakeDeviceSettingName(buffer, "Port", n, "IgnoreChecksum");
  Set(buffer, config.ignore_checksum);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Bus");
  Set(buffer, config.i2c_bus);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Addr");
  Set(buffer, config.i2c_addr);

  MakeDeviceSettingName(buffer, "Port", n, "PressureUse");
  SetEnum(buffer, config.press_use);

  MakeDeviceSettingName(buffer, "Port", n, "SensorOffset");
  fixed offset = DeviceConfig::UsesCalibration(config.port_type) ? config.sensor_offset : fixed(0);
  // Has new calibration data been delivered ?
  if (CommonInterface::Basic().sensor_calibration_available)
    offset = CommonInterface::Basic().sensor_calibration_offset;
  Set(buffer, offset);

  MakeDeviceSettingName(buffer, "Port", n, "SensorFactor");
  fixed factor = DeviceConfig::UsesCalibration(config.port_type) ? config.sensor_factor : fixed(0);
  // Has new calibration data been delivered ?
  if (CommonInterface::Basic().sensor_calibration_available)
    factor = CommonInterface::Basic().sensor_calibration_factor;
  Set(buffer, factor);
}
