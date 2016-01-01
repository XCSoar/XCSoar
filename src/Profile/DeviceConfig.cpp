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

#include "DeviceConfig.hpp"
#include "Map.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Device/Config.hpp"

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#include "Java/Global.hxx"
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
  "tcp_client",
  "tcp_listener",
  "udp_listener",
  "pty",
  NULL
};

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
ReadPortType(const ProfileMap &map, unsigned n, DeviceConfig::PortType &type)
{
  char name[64];

  MakeDeviceSettingName(name, "Port", n, "Type");

  const char *value = map.Get(name);
  return value != NULL && StringToPortType(value, type);
}

static bool
LoadPath(const ProfileMap &map, DeviceConfig &config, unsigned n)
{
  char buffer[64];
  MakeDeviceSettingName(buffer, "Port", n, "Path");
  return map.Get(buffer, config.path);
}

static bool
LoadPortIndex(const ProfileMap &map, DeviceConfig &config, unsigned n)
{
  char buffer[64];
  MakeDeviceSettingName(buffer, "Port", n, "Index");

  unsigned index;
  if (!map.Get(buffer, index))
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
Profile::GetDeviceConfig(const ProfileMap &map, unsigned n,
                         DeviceConfig &config)
{
  char buffer[64];

  bool have_port_type = ReadPortType(map, n, config.port_type);

  MakeDeviceSettingName(buffer, "Port", n, "BluetoothMAC");
  map.Get(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, "Port", n, "IOIOUartID");
  map.Get(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, "Port", n, "IPAddress");
  if (!map.Get(buffer, config.ip_address))
    config.ip_address.clear();

  MakeDeviceSettingName(buffer, "Port", n, "TCPPort");
  if (!map.Get(buffer, config.tcp_port))
    config.tcp_port = 4353;

  config.path.clear();
  if ((!have_port_type ||
       config.port_type == DeviceConfig::PortType::SERIAL) &&
      !LoadPath(map, config, n) && LoadPortIndex(map, config, n))
    config.port_type = DeviceConfig::PortType::SERIAL;

  MakeDeviceSettingName(buffer, "Port", n, "BaudRate");
  if (!map.Get(buffer, config.baud_rate)) {
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
    if (map.Get(buffer, speed_index) &&
        speed_index < ARRAY_SIZE(speed_index_table))
      config.baud_rate = speed_index_table[speed_index];
  }

  MakeDeviceSettingName(buffer, "Port", n, "BulkBaudRate");
  if (!map.Get(buffer, config.bulk_baud_rate))
    config.bulk_baud_rate = 0;

  strcpy(buffer, "DeviceA");
  buffer[strlen(buffer) - 1] += n;
  map.Get(buffer, config.driver_name);

  MakeDeviceSettingName(buffer, "Port", n, "Enabled");
  map.Get(buffer, config.enabled);

  MakeDeviceSettingName(buffer, "Port", n, "SyncFromDevice");
  map.Get(buffer, config.sync_from_device);

  MakeDeviceSettingName(buffer, "Port", n, "SyncToDevice");
  map.Get(buffer, config.sync_to_device);

  MakeDeviceSettingName(buffer, "Port", n, "K6Bt");
  map.Get(buffer, config.k6bt);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Bus");
  map.Get(buffer, config.i2c_bus);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Addr");
  map.Get(buffer, config.i2c_addr);

  MakeDeviceSettingName(buffer, "Port", n, "PressureUse");
  map.GetEnum(buffer, config.press_use);

  MakeDeviceSettingName(buffer, "Port", n, "SensorOffset");
  map.Get(buffer, config.sensor_offset);

  MakeDeviceSettingName(buffer, "Port", n, "SensorFactor");
  map.Get(buffer, config.sensor_factor);

  MakeDeviceSettingName(buffer, "Port", n, "UseSecondDevice");
  map.Get(buffer, config.use_second_device);

  MakeDeviceSettingName(buffer, "Port", n, "SecondDevice");
  map.Get(buffer, config.driver2_name);
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
WritePortType(ProfileMap &map, unsigned n, DeviceConfig::PortType type)
{
  const char *value = PortTypeToString(type);
  if (value == NULL)
    return;

  char name[64];
  MakeDeviceSettingName(name, "Port", n, "Type");
  map.Set(name, value);
}

void
Profile::SetDeviceConfig(ProfileMap &map,
                         unsigned n, const DeviceConfig &config)
{
  char buffer[64];

  WritePortType(map, n, config.port_type);

  MakeDeviceSettingName(buffer, "Port", n, "BluetoothMAC");
  map.Set(buffer, config.bluetooth_mac);

  MakeDeviceSettingName(buffer, "Port", n, "IOIOUartID");
  map.Set(buffer, config.ioio_uart_id);

  MakeDeviceSettingName(buffer, "Port", n, "Path");
  map.Set(buffer, config.path);

  MakeDeviceSettingName(buffer, "Port", n, "BaudRate");
  map.Set(buffer, config.baud_rate);

  MakeDeviceSettingName(buffer, "Port", n, "BulkBaudRate");
  map.Set(buffer, config.bulk_baud_rate);

  MakeDeviceSettingName(buffer, "Port", n, "IPAddress");
  map.Set(buffer, config.ip_address);

  MakeDeviceSettingName(buffer, "Port", n, "TCPPort");
  map.Set(buffer, config.tcp_port);

  strcpy(buffer, "DeviceA");
  buffer[strlen(buffer) - 1] += n;
  map.Set(buffer, config.driver_name);

  MakeDeviceSettingName(buffer, "Port", n, "Enabled");
  map.Set(buffer, config.enabled);

  MakeDeviceSettingName(buffer, "Port", n, "SyncFromDevice");
  map.Set(buffer, config.sync_from_device);

  MakeDeviceSettingName(buffer, "Port", n, "SyncToDevice");
  map.Set(buffer, config.sync_to_device);

  MakeDeviceSettingName(buffer, "Port", n, "K6Bt");
  map.Set(buffer, config.k6bt);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Bus");
  map.Set(buffer, config.i2c_bus);

  MakeDeviceSettingName(buffer, "Port", n, "I2C_Addr");
  map.Set(buffer, config.i2c_addr);

  MakeDeviceSettingName(buffer, "Port", n, "PressureUse");
  map.SetEnum(buffer, config.press_use);

  MakeDeviceSettingName(buffer, "Port", n, "SensorOffset");
  auto offset = DeviceConfig::UsesCalibration(config.port_type) ? config.sensor_offset : fixed(0);
  // Has new calibration data been delivered ?
  if (CommonInterface::Basic().sensor_calibration_available)
    offset = CommonInterface::Basic().sensor_calibration_offset;
  map.Set(buffer, offset);

  MakeDeviceSettingName(buffer, "Port", n, "SensorFactor");
  auto factor = DeviceConfig::UsesCalibration(config.port_type) ? config.sensor_factor : fixed(0);
  // Has new calibration data been delivered ?
  if (CommonInterface::Basic().sensor_calibration_available)
    factor = CommonInterface::Basic().sensor_calibration_factor;
  map.Set(buffer, factor);

  MakeDeviceSettingName(buffer, "Port", n, "UseSecondDevice");
  map.Set(buffer, config.use_second_device);

  MakeDeviceSettingName(buffer, "Port", n, "SecondDevice");
  map.Set(buffer, config.driver2_name);
}
