// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceConfig.hpp"
#include "Map.hpp"
#include "util/Macros.hpp"
#include "util/StringFormat.hpp"
#include "Interface.hpp"
#include "Device/Config.hpp"

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#include "java/Global.hxx"
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
  "ble_sensor",
  "ble_hm10",
  "glider_link",
  "android_usb_serial",
  NULL
};

[[nodiscard]]
static const char *
MakeDeviceSettingName(char *buffer, size_t buffer_size,
                      const char *prefix, unsigned n,
                      const char *suffix)
{
  if (prefix == nullptr)
    prefix = "";
  if (suffix == nullptr)
    suffix = "";

  const int written = n > 0
    ? StringFormat(buffer, buffer_size, "%s%u%s", prefix, n + 1, suffix)
    : StringFormat(buffer, buffer_size, "%s%s", prefix, suffix);

  if (written < 0 || (size_t)written >= buffer_size)
    return nullptr;

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

  if (MakeDeviceSettingName(name, sizeof(name), "Port", n, "Type") == nullptr)
    return false;

  const char *value = map.Get(name);
  return value != NULL && StringToPortType(value, type);
}

static bool
LoadPath(const ProfileMap &map, DeviceConfig &config, unsigned n)
{
  char buffer[64];
  if (MakeDeviceSettingName(buffer, sizeof(buffer), "Port", n, "Path") == nullptr)
    return false;

  return map.Get(buffer, config.path);
}

static bool
LoadPortIndex(const ProfileMap &map, DeviceConfig &config, unsigned n)
{
  char buffer[64];
  if (MakeDeviceSettingName(buffer, sizeof(buffer), "Port", n, "Index") == nullptr)
    return false;

  unsigned index;
  if (!map.Get(buffer, index))
    return false;

  /* adjust the number, compatibility quirk for XCSoar 5 */
  if (index < 10)
    ++index;
  else if (index == 10)
    index = 0;

  char path[64];
  const int written = StringFormat(path, sizeof(path), "COM%u:", index);
  if (written < 0 || static_cast<size_t>(written) >= sizeof(path))
    return false;

  config.path = path;
  return true;
}

void
Profile::GetDeviceConfig(const ProfileMap &map, unsigned n,
                         DeviceConfig &config)
{
  char buffer[64];
  const auto make_port_name = [&](const char *suffix) {
    return MakeDeviceSettingName(buffer, sizeof(buffer), "Port", n, suffix);
  };

  bool have_port_type = ReadPortType(map, n, config.port_type);

  if (const char *name = make_port_name("BluetoothMAC"); name != nullptr)
    map.Get(name, config.bluetooth_mac);

  if (const char *name = make_port_name("IOIOUartID"); name != nullptr)
    map.Get(name, config.ioio_uart_id);

  if (const char *name = make_port_name("IPAddress");
      name == nullptr || !map.Get(name, config.ip_address))
    config.ip_address.clear();

  if (const char *name = make_port_name("TCPPort");
      name == nullptr || !map.Get(name, config.tcp_port))
    config.tcp_port = 4353;

  config.path.clear();
  if ((!have_port_type ||
       config.port_type == DeviceConfig::PortType::ANDROID_USB_SERIAL ||
       config.port_type == DeviceConfig::PortType::SERIAL) &&
      !LoadPath(map, config, n) && LoadPortIndex(map, config, n))
    config.port_type = DeviceConfig::PortType::SERIAL;

  if (const char *name = make_port_name("BaudRate");
      name == nullptr || !map.Get(name, config.baud_rate)) {
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

    unsigned speed_index;
    if (const char *speed_name =
          MakeDeviceSettingName(buffer, sizeof(buffer), "Speed", n, "Index");
        speed_name != nullptr && map.Get(speed_name, speed_index) &&
        speed_index < ARRAY_SIZE(speed_index_table))
      config.baud_rate = speed_index_table[speed_index];
  }

  if (const char *name = make_port_name("BulkBaudRate");
      name == nullptr || !map.Get(name, config.bulk_baud_rate))
    config.bulk_baud_rate = 0;

  strcpy(buffer, "DeviceA");
  buffer[strlen(buffer) - 1] += n;
  map.Get(buffer, config.driver_name);

  if (const char *name = make_port_name("Enabled"); name != nullptr)
    map.Get(name, config.enabled);

  if (const char *name = make_port_name("SyncFromDevice"); name != nullptr)
    map.Get(name, config.sync_from_device);

  if (const char *name = make_port_name("SyncToDevice"); name != nullptr)
    map.Get(name, config.sync_to_device);

  if (const char *name = make_port_name("K6Bt"); name != nullptr)
    map.Get(name, config.k6bt);

  if (const char *name = make_port_name("I2C_Bus"); name != nullptr)
    map.Get(name, config.i2c_bus);

  if (const char *name = make_port_name("I2C_Addr"); name != nullptr)
    map.Get(name, config.i2c_addr);

  if (const char *name = make_port_name("PressureUse"); name != nullptr)
    map.GetEnum(name, config.press_use);

  if (const char *name = make_port_name("SensorOffset"); name != nullptr)
    map.Get(name, config.sensor_offset);

  if (const char *name = make_port_name("SensorFactor"); name != nullptr)
    map.Get(name, config.sensor_factor);

  if (const char *name = make_port_name("UseSecondDevice"); name != nullptr)
    map.Get(name, config.use_second_device);

  if (const char *name = make_port_name("SecondDevice"); name != nullptr)
    map.Get(name, config.driver2_name);

  if (const char *name = make_port_name("EngineType");
      name == nullptr || !map.GetEnum(name, config.engine_type) ||
      unsigned(config.engine_type) >= unsigned(DeviceConfig::EngineType::MAX))
    config.engine_type = DeviceConfig::EngineType::NONE;

  if (const char *name = make_port_name("PolarSync");
      name == nullptr || !map.GetEnum(name, config.polar_sync) ||
      unsigned(config.polar_sync) >= unsigned(DeviceConfig::PolarSync::COUNT))
    config.polar_sync = DeviceConfig::PolarSync::OFF;
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
  if (MakeDeviceSettingName(name, sizeof(name), "Port", n, "Type") == nullptr)
    return;

  map.Set(name, value);
}

void
Profile::SetDeviceConfig(ProfileMap &map,
                         unsigned n, const DeviceConfig &config)
{
  char buffer[64];
  const auto make_port_name = [&](const char *suffix) {
    return MakeDeviceSettingName(buffer, sizeof(buffer), "Port", n, suffix);
  };

  WritePortType(map, n, config.port_type);

  if (const char *name = make_port_name("BluetoothMAC"); name != nullptr)
    map.Set(name, config.bluetooth_mac);

  if (const char *name = make_port_name("IOIOUartID"); name != nullptr)
    map.Set(name, config.ioio_uart_id);

  if (const char *name = make_port_name("Path"); name != nullptr)
    map.Set(name, config.path);

  if (const char *name = make_port_name("BaudRate"); name != nullptr)
    map.Set(name, config.baud_rate);

  if (const char *name = make_port_name("BulkBaudRate"); name != nullptr)
    map.Set(name, config.bulk_baud_rate);

  if (const char *name = make_port_name("IPAddress"); name != nullptr)
    map.Set(name, config.ip_address);

  if (const char *name = make_port_name("TCPPort"); name != nullptr)
    map.Set(name, config.tcp_port);

  strcpy(buffer, "DeviceA");
  buffer[strlen(buffer) - 1] += n;
  map.Set(buffer, config.driver_name);

  if (const char *name = make_port_name("Enabled"); name != nullptr)
    map.Set(name, config.enabled);

  if (const char *name = make_port_name("SyncFromDevice"); name != nullptr)
    map.Set(name, config.sync_from_device);

  if (const char *name = make_port_name("SyncToDevice"); name != nullptr)
    map.Set(name, config.sync_to_device);

  if (const char *name = make_port_name("K6Bt"); name != nullptr)
    map.Set(name, config.k6bt);

  if (const char *name = make_port_name("I2C_Bus"); name != nullptr)
    map.Set(name, config.i2c_bus);

  if (const char *name = make_port_name("I2C_Addr"); name != nullptr)
    map.Set(name, config.i2c_addr);

  if (const char *name = make_port_name("PressureUse"); name != nullptr)
    map.SetEnum(name, config.press_use);

  auto offset = DeviceConfig::UsesCalibration(config.port_type) ? config.sensor_offset : 0;
  // Has new calibration data been delivered ?
  if (CommonInterface::Basic().sensor_calibration_available)
    offset = CommonInterface::Basic().sensor_calibration_offset;
  if (const char *name = make_port_name("SensorOffset"); name != nullptr)
    map.Set(name, offset);

  auto factor = DeviceConfig::UsesCalibration(config.port_type) ? config.sensor_factor : 0;
  // Has new calibration data been delivered ?
  if (CommonInterface::Basic().sensor_calibration_available)
    factor = CommonInterface::Basic().sensor_calibration_factor;
  if (const char *name = make_port_name("SensorFactor"); name != nullptr)
    map.Set(name, factor);

  if (const char *name = make_port_name("UseSecondDevice"); name != nullptr)
    map.Set(name, config.use_second_device);

  if (const char *name = make_port_name("SecondDevice"); name != nullptr)
    map.Set(name, config.driver2_name);

  if (const char *name = make_port_name("EngineType"); name != nullptr)
    map.SetEnum(name, config.engine_type);

  if (const char *name = make_port_name("PolarSync"); name != nullptr)
    map.SetEnum(name, config.polar_sync);
}
