// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceEditWidget.hpp"
#include "PortDataField.hpp"
#include "PortPicker.hpp"
#include "Dialogs/GroupedListPicker.hpp"
#include "Dialogs/TextEntry.hpp"
#include "UIGlobals.hpp"
#include "util/Compiler.h"
#include "util/NumberParser.hpp"
#include "util/StringAPI.hxx"
#include "Language/Language.hpp"
#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Interface.hpp"

#ifdef ANDROID
#include "java/Global.hxx"
#include "Android/Main.hpp"
#include "Android/BluetoothHelper.hpp"
#endif

#include <algorithm>
#include <span>
#include <vector>

namespace {

struct IdChoice {
  unsigned id;
  const char *caption;
};

static constexpr IdChoice baud_rates[] = {
  {1200, "1200"},
  {2400, "2400"},
  {4800, "4800"},
  {9600, "9600"},
  {19200, "19200"},
  {38400, "38400"},
  {57600, "57600"},
  {115200, "115200"},
  {230400, "230400"},
  {256000, "256000"},
  {460800, "460800"},
  {500000, "500000"},
  {921600, "921600"},
  {1000000, "1000000"},
};

static constexpr IdChoice bulk_rates[] = {
  {0, "Default"},
  {1200, "1200"},
  {2400, "2400"},
  {4800, "4800"},
  {9600, "9600"},
  {19200, "19200"},
  {38400, "38400"},
  {57600, "57600"},
  {115200, "115200"},
  {230400, "230400"},
  {256000, "256000"},
  {460800, "460800"},
  {500000, "500000"},
  {921600, "921600"},
  {1000000, "1000000"},
};

static constexpr IdChoice tcp_ports[] = {
  {55278, "55278 (Condor UDP)"},
  {4353, "4353"},
  {10110, "10110"},
  {4352, "4352"},
  {2000, "2000"},
  {4000, "4000"},
  {23, "23"},
  {8880, "8880"},
  {8881, "8881"},
  {8882, "8882"},
};

static constexpr IdChoice i2c_buses[] = {
  {0, "0"},
  {1, "1"},
  {2, "2"},
};

static constexpr IdChoice i2c_addrs[] = {
  {0x76, "0x76 (MS5611)"},
  {0x77, "0x77 (BMP085 and MS5611)"},
};

static constexpr IdChoice pressure_uses[] = {
  {unsigned(DeviceConfig::PressureUse::STATIC_WITH_VARIO),
   "Static & Vario"},
  {unsigned(DeviceConfig::PressureUse::STATIC_ONLY), "Static"},
  {unsigned(DeviceConfig::PressureUse::TEK_PRESSURE),
   "TE probe (compensated vario)"},
  {unsigned(DeviceConfig::PressureUse::PITOT), "Pitot (airspeed)"},
};

static constexpr IdChoice engine_types[] = {
  {unsigned(DeviceConfig::EngineType::NONE), "None"},
  {unsigned(DeviceConfig::EngineType::TWO_STROKE_1_IGN), "2S1I"},
  {unsigned(DeviceConfig::EngineType::TWO_STROKE_2_IGN), "2S2I"},
  {unsigned(DeviceConfig::EngineType::FOUR_STROKE_1_IGN), "4S1I"},
};

[[gnu::pure]]
static const char *
IdCaption(std::span<const IdChoice> list, unsigned id) noexcept
{
  for (const auto &choice : list)
    if (choice.id == id)
      return choice.caption;

  return nullptr;
}

static bool
PickId(const char *caption, const char *help,
       std::span<const IdChoice> list, unsigned &value) noexcept
{
  std::vector<PickerChoice> choices;
  choices.reserve(list.size());

  int current = -1;
  for (unsigned i = 0; i < list.size(); ++i) {
    choices.push_back({list[i].caption});
    if (list[i].id == value)
      current = int(i);
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || list[picked].id == value)
    return false;

  value = list[picked].id;
  return true;
}

template<std::size_t N>
static bool
EditText(StaticString<N> &value, const char *caption) noexcept
{
  StaticString<N> edited = value;
  if (!TextEntryDialog(edited.data(), edited.capacity(), caption))
    return false;

  if (StringIsEqual(edited, value))
    return false;

  value = edited;
  return true;
}

[[gnu::pure]]
static const DeviceRegister &
DriverOf(const char *name) noexcept
{
  return *FindDriverByName(name != nullptr ? name : "");
}

[[gnu::pure]]
static const char *
DriverLabel(const DeviceRegister &driver) noexcept
{
  return driver.display_name != nullptr
    ? driver.display_name
    : driver.name;
}

/**
 * Engine Type is only for the PPG engine-sensor GATT service, not for
 * heart-rate BLE sensors (e.g. a Xiaomi band).
 */
[[gnu::pure]]
static bool
ShowsEngineType(DeviceConfig::PortType type,
                DeviceConfig::EngineType engine_type,
                [[maybe_unused]] const char *bluetooth_mac) noexcept
{
  if (type != DeviceConfig::PortType::BLE_SENSOR)
    return false;

  if (engine_type != DeviceConfig::EngineType::NONE)
    return true;

#ifdef ANDROID
  if (bluetooth_helper != nullptr &&
      bluetooth_mac != nullptr && bluetooth_mac[0] != '\0')
    return bluetooth_helper->HasEngineSensors(Java::GetEnv(),
                                              bluetooth_mac);
#endif

  return false;
}

struct Shown {
  bool baud;
  bool bulk;
  bool ip;
  bool tcp;
  bool spectate;
  bool i2c_bus;
  bool i2c_addr;
  bool pressure;
  bool driver;
  bool passthrough;
  bool second_driver;
  bool sync_from;
  bool sync_to;
  bool send_position;
  bool polar;
  bool receive_polar;
  bool send_polar;
  bool k6bt;
  bool engine;
};

[[gnu::pure]]
static Shown
WhatIsShown(const DeviceConfig &config,
            const char *port_string) noexcept
{
  const auto type = config.port_type;
  const auto &driver = DriverOf(config.driver_name);
  const bool uses_driver = DeviceConfig::UsesDriver(type);
  const bool maybe_bluetooth =
    DeviceConfig::MaybeBluetooth(type, port_string);
  const bool can_pass = driver.HasPassThrough();

  Shown shown{};
  shown.baud = DeviceConfig::UsesSpeed(type) ||
    (maybe_bluetooth && config.k6bt);
  shown.bulk = shown.baud && uses_driver && driver.SupportsBulkBaudRate();
  shown.ip = DeviceConfig::UsesIPAddress(type);
  shown.tcp = DeviceConfig::UsesTCPPort(type);
  shown.spectate = type == DeviceConfig::PortType::SPECTATE_FILE;
  shown.i2c_bus = DeviceConfig::UsesI2C(type);
  shown.i2c_addr = shown.i2c_bus &&
    type != DeviceConfig::PortType::NUNCHUCK;
  shown.pressure = DeviceConfig::IsPressureSensor(type);
  shown.driver = uses_driver;
  shown.passthrough = uses_driver && can_pass;
  shown.second_driver = shown.passthrough && config.use_second_device;
  shown.sync_from = uses_driver && driver.CanReceiveSettings();
  shown.sync_to = uses_driver && driver.CanSendSettings();
  shown.send_position = uses_driver && driver.CanSendPosition();
  shown.receive_polar = uses_driver && driver.CanReceivePolar();
  shown.send_polar = uses_driver && driver.CanSendPolar();
  shown.polar = shown.receive_polar || shown.send_polar;
  shown.k6bt = maybe_bluetooth;
  shown.engine = ShowsEngineType(type, config.engine_type, port_string);
  return shown;
}

static DeviceConfig::PolarSync
ShownPolar(DeviceConfig::PolarSync mode,
           bool receive, bool send) noexcept
{
  if (mode == DeviceConfig::PolarSync::RECEIVE && receive)
    return mode;

  if (mode == DeviceConfig::PolarSync::SEND && send)
    return mode;

  return DeviceConfig::PolarSync::OFF;
}

[[gnu::pure]]
static const char *
PolarCaption(DeviceConfig::PolarSync mode) noexcept
{
  switch (mode) {
  case DeviceConfig::PolarSync::RECEIVE:
    return _("Receive from device");

  case DeviceConfig::PolarSync::SEND:
    return _("Send to device");

  case DeviceConfig::PolarSync::OFF:
  case DeviceConfig::PolarSync::COUNT:
    break;
  }

  return _("Off");
}

/**
 * @return true if the value has changed
 */
static bool
FinishPortField(DeviceConfig &config, const DataFieldEnum &df) noexcept
{
  unsigned value = df.GetValue();

  /* decode the port type from the upper 16 bits of the id; we don't
     need the rest, because that's just some serial we don't care
     about */
  const DeviceConfig::PortType new_type =
    (DeviceConfig::PortType)(value >> 16);
  switch (new_type) {
  case DeviceConfig::PortType::DISABLED:
  case DeviceConfig::PortType::INTERNAL:
  case DeviceConfig::PortType::DROIDSOAR_V2:
  case DeviceConfig::PortType::NUNCHUCK:
  case DeviceConfig::PortType::I2CPRESSURESENSOR:
  case DeviceConfig::PortType::IOIOVOLTAGE:
  case DeviceConfig::PortType::TCP_CLIENT:
  case DeviceConfig::PortType::TCP_LISTENER:
  case DeviceConfig::PortType::UDP_LISTENER:
  case DeviceConfig::PortType::RFCOMM_SERVER:
  case DeviceConfig::PortType::GLIDER_LINK:
  case DeviceConfig::PortType::SPECTATE_FILE:
    if (new_type == config.port_type)
      return false;

    /* Drop a stale serial path (e.g. COMx:) before applying Spectate
       defaults; otherwise ApplySpectateDefaults leaves it unchanged. */
    if (new_type == DeviceConfig::PortType::SPECTATE_FILE)
      config.path.clear();

    config.port_type = new_type;
    if (new_type == DeviceConfig::PortType::SPECTATE_FILE)
      config.ApplySpectateDefaults();
    return true;

  case DeviceConfig::PortType::SERIAL:
  case DeviceConfig::PortType::PTY:
  case DeviceConfig::PortType::ANDROID_USB_SERIAL:
    if (new_type == config.port_type &&
        StringIsEqual(config.path, df.GetAsString()))
      return false;

    config.port_type = new_type;
    config.path = df.GetAsString();
    return true;

  case DeviceConfig::PortType::RFCOMM:
  case DeviceConfig::PortType::BLE_SERIAL:
  case DeviceConfig::PortType::BLE_SENSOR:
    if (new_type == config.port_type &&
        StringIsEqual(config.bluetooth_mac, df.GetAsString()))
      return false;

    config.port_type = new_type;
    config.bluetooth_mac = df.GetAsString();
    return true;

  case DeviceConfig::PortType::IOIOUART:
    if (new_type == config.port_type &&
        config.ioio_uart_id == (unsigned)ParseUnsigned(df.GetAsString()))
      return false;

    config.port_type = new_type;
    config.ioio_uart_id = (unsigned)ParseUnsigned(df.GetAsString());
    return true;
  }

  gcc_unreachable();
  assert(false);
  return false;
}

static bool
AssignString(StaticString<64> &dest, const StaticString<64> &value) noexcept
{
  if (StringIsEqual(dest, value))
    return false;

  dest = value;
  return true;
}

static bool
AssignCallsign(StaticString<128> &dest,
               const StaticString<128> &value) noexcept
{
  if (StringIsEqual(dest, value))
    return false;

  dest = value;
  return true;
}

} // namespace

DeviceEditWidget::DeviceEditWidget(const DeviceConfig &_config) noexcept
  :GroupedListWidget(UIGlobals::GetDialogLook()),
   config(_config),
   baseline(_config),
   port_df(nullptr)
{
  FillPorts(port_df, config);
}

void
DeviceEditWidget::SetConfig(const DeviceConfig &_config) noexcept
{
  config = _config;

  if (config.port_type == DeviceConfig::PortType::DISABLED)
    /* if the user configures a new device, forget the old "enabled"
       flag and re-enable the device */
    config.enabled = true;

  if (config.port_type == DeviceConfig::PortType::SPECTATE_FILE)
    config.ApplySpectateDefaults();

  baseline = config;
  SetPort(port_df, config);

  if (GetItemCount() == 0)
    return;

  SetItemChecked(passthrough_item, config.use_second_device);
  SetItemChecked(sync_from_item, config.sync_from_device);
  SetItemChecked(sync_to_item, config.sync_to_device);
  SetItemChecked(send_position_item, config.send_position);
  SetItemChecked(k6bt_item, config.k6bt);
  UpdateVisibilities();
}

void
DeviceEditWidget::UpdateVisibilities() noexcept
{
  UpdateValues();
}

void
DeviceEditWidget::Notify() noexcept
{
  if (listener != nullptr)
    listener->OnModified(*this);
}

void
DeviceEditWidget::PickPort() noexcept
{
  if (!PortPicker(port_df, _("Port")))
    return;

  const auto type = GetPortType(port_df);
  if (type == DeviceConfig::PortType::SPECTATE_FILE) {
    if (config.port_type != DeviceConfig::PortType::SPECTATE_FILE)
      config.path.clear();

    config.ApplySpectateDefaults();
  }

  FinishPortField(config, port_df);
  UpdateVisibilities();
  Notify();
}

void
DeviceEditWidget::PickDriver(bool second) noexcept
{
  std::vector<const DeviceRegister *> drivers;
  for (unsigned i = 0;; ++i) {
    const auto *driver = GetDriverByIndex(i);
    if (driver == nullptr)
      break;

    drivers.push_back(driver);
  }

  if (drivers.size() > 1)
    std::sort(std::next(drivers.begin()), drivers.end(),
              [](const DeviceRegister *a, const DeviceRegister *b) {
                return StringCollate(DriverLabel(*a), DriverLabel(*b)) < 0;
              });

  std::vector<PickerChoice> choices;
  choices.reserve(drivers.size());

  const char *selected = second
    ? config.driver2_name.c_str()
    : config.driver_name.c_str();
  int current = -1;
  for (unsigned i = 0; i < drivers.size(); ++i) {
    choices.push_back({DriverLabel(*drivers[i])});
    if (StringIsEqual(drivers[i]->name, selected))
      current = int(i);
  }

  const char *caption = second ? _("Second Driver") : _("Driver");
  const int picked = PickChoice(caption, nullptr, choices, current);
  if (picked < 0 || StringIsEqual(drivers[picked]->name, selected))
    return;

  if (second)
    config.driver2_name = drivers[picked]->name;
  else {
    config.driver_name = drivers[picked]->name;
    const auto &driver = DriverOf(config.driver_name);
    config.polar_sync = ShownPolar(config.polar_sync,
                                   driver.CanReceivePolar(),
                                   driver.CanSendPolar());
  }

  UpdateVisibilities();
  Notify();
}

void
DeviceEditWidget::Fill() noexcept
{
  AddGroup();

  AddItem(_("Port"), [this]{ PickPort(); }, {
    .chevron = true,
    .value_callback = [this](ValueState &state) {
      const char *label = port_df.GetAsDisplayString();
      state.text = label != nullptr ? label : "";
    },
  });

  AddValue(_("Engine Type"), nullptr,
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.engine;
             const char *caption =
               IdCaption(engine_types, unsigned(config.engine_type));
             state.text = caption != nullptr ? caption : "None";
           },
           [this]{
             unsigned value = unsigned(config.engine_type);
             if (!PickId(_("Engine Type"), nullptr, engine_types, value))
               return;

             config.engine_type = DeviceConfig::EngineType(value);
             UpdateVisibilities();
             Notify();
           });

  AddValue(_("Baud rate"), nullptr,
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.baud;
             const char *caption = IdCaption(baud_rates, config.baud_rate);
             if (caption != nullptr)
               state.text = caption;
             else {
               StaticString<16> fallback;
               fallback.Format("%u", config.baud_rate);
               state.text = fallback.c_str();
             }
           },
           [this]{
             if (!PickId(_("Baud rate"), nullptr, baud_rates,
                         config.baud_rate))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(_("Bulk baud rate"),
           _("The baud rate used for bulk transfers, such as task "
             "declaration or flight download."),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.bulk;
             const char *caption =
               IdCaption(bulk_rates, config.bulk_baud_rate);
             state.text = caption != nullptr ? caption : "Default";
           },
           [this]{
             if (!PickId(_("Bulk baud rate"),
                         _("The baud rate used for bulk transfers, such as "
                           "task declaration or flight download."),
                         bulk_rates, config.bulk_baud_rate))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(_("IP address"), nullptr,
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.ip;
             state.text = config.ip_address.c_str();
           },
           [this]{
             if (!EditText(config.ip_address, _("IP address")))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(_("TCP port"), nullptr,
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.tcp;
             const char *caption = IdCaption(tcp_ports, config.tcp_port);
             if (caption != nullptr)
               state.text = caption;
             else {
               StaticString<16> fallback;
               fallback.Format("%u", config.tcp_port);
               state.text = fallback.c_str();
             }
           },
           [this]{
             if (!PickId(_("TCP port"), nullptr, tcp_ports, config.tcp_port))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(C_("Setting", "Spectate file"),
           _("Full path to Condor 3 Spectate.json. "
             "Default: c:\\condor3\\logs\\spectate.json"),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.spectate;
             state.text = config.path.c_str();
           },
           [this]{
             if (!EditText(config.path, C_("Setting", "Spectate file")))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(C_("Setting", "Own callsign"),
           _("Competition number of your glider in Spectate.json "
             "(excluded from traffic, used as position reference)."),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.spectate;
             state.text = config.port_name.c_str();
           },
           [this]{
             if (!EditText(config.port_name,
                           C_("Setting", "Own callsign")))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(_("I²C bus"),
           _("Select the description or bus number that matches your "
             "configuration."),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.i2c_bus;
             const char *caption = IdCaption(i2c_buses, config.i2c_bus);
             state.text = caption != nullptr ? caption : "0";
           },
           [this]{
             if (!PickId(_("I²C bus"),
                         _("Select the description or bus number that "
                           "matches your configuration."),
                         i2c_buses, config.i2c_bus))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(_("I²C addr"),
           _("The I²C address that matches your configuration. "
             "This field is not used when your selection in the \"I²C bus\" "
             "field is not an I²C bus number. "
             "Assume this field is not in use if that doesn't make sense "
             "to you."),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.i2c_addr;
             const char *caption = IdCaption(i2c_addrs, config.i2c_addr);
             state.text = caption != nullptr ? caption : "";
           },
           [this]{
             if (!PickId(_("I²C addr"),
                         _("The I²C address that matches your configuration. "
                           "This field is not used when your selection in "
                           "the \"I²C bus\" field is not an I²C bus number. "
                           "Assume this field is not in use if that doesn't "
                           "make sense to you."),
                         i2c_addrs, config.i2c_addr))
               return;

             UpdateValues();
             Notify();
           });

  AddValue(_("Pressure use"),
           _("Select the purpose of this pressure sensor. "
             "This sensor measures some pressure. Here you tell the system "
             "what pressure this is and what it should be used for."),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.pressure;
             const char *caption =
               IdCaption(pressure_uses, unsigned(config.press_use));
             state.text = caption != nullptr ? caption : "";
           },
           [this]{
             unsigned value = unsigned(config.press_use);
             if (!PickId(_("Pressure use"),
                         _("Select the purpose of this pressure sensor. "
                           "This sensor measures some pressure. Here you "
                           "tell the system what pressure this is and what "
                           "it should be used for."),
                         pressure_uses, value))
               return;

             config.press_use = DeviceConfig::PressureUse(value);
             UpdateValues();
             Notify();
           });

  AddValue(_("Driver"), nullptr,
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.driver;
             const char *label =
               FindDriverDisplayName(config.driver_name.c_str());
             state.text = label != nullptr
               ? label
               : config.driver_name.c_str();
           },
           [this]{ PickDriver(false); });

  passthrough_item = GetItemCount();
  AddItem(_("Passthrough device"), [this]{
    config.use_second_device = IsItemChecked(passthrough_item);
    UpdateVisibilities();
    Notify();
  }, {
    .toggle = true,
    .checked = config.use_second_device,
    .help = _("Whether the device has a passed-"
              "through device connected."),
    .value_callback = [this](ValueState &state) {
      const auto shown = WhatIsShown(config, port_df.GetAsString());
      state.hidden = !shown.passthrough;
    },
  });

  AddValue(_("Second Driver"), nullptr,
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.second_driver;
             const char *label =
               FindDriverDisplayName(config.driver2_name.c_str());
             state.text = label != nullptr
               ? label : config.driver2_name.c_str();
           },
           [this]{ PickDriver(true); });

  sync_from_item = GetItemCount();
  AddItem(_("Sync. from device"), [this]{
    config.sync_from_device = IsItemChecked(sync_from_item);
    UpdateValues();
    Notify();
  }, {
    .toggle = true,
    .checked = config.sync_from_device,
    .help = _("Tells XCSoar to use settings "
              "like the MacCready value, bugs and ballast from the device."),
    .value_callback = [this](ValueState &state) {
      const auto shown = WhatIsShown(config, port_df.GetAsString());
      state.hidden = !shown.sync_from;
    },
    .expert = true,
  });

  sync_to_item = GetItemCount();
  AddItem(_("Sync. to device"), [this]{
    config.sync_to_device = IsItemChecked(sync_to_item);
    UpdateValues();
    Notify();
  }, {
    .toggle = true,
    .checked = config.sync_to_device,
    .help = _("Tells XCSoar to send settings "
              "like the MacCready value, bugs and ballast to the device."),
    .value_callback = [this](ValueState &state) {
      const auto shown = WhatIsShown(config, port_df.GetAsString());
      state.hidden = !shown.sync_to;
    },
    .expert = true,
  });

  send_position_item = GetItemCount();
  AddItem(C_("Setting", "Emit GPGGA/GPRMC"), [this]{
    config.send_position = IsItemChecked(send_position_item);
    UpdateValues();
    Notify();
  }, {
    .toggle = true,
    .checked = config.send_position,
    .help = _("Tells XCSoar to send its current GPS position to the "
              "device as $GPGGA and $GPRMC sentences. Turn off when "
              "another GPS source is already feeding the device on "
              "the same line. Changes take effect after reconnecting "
              "the device."),
    .value_callback = [this](ValueState &state) {
      const auto shown = WhatIsShown(config, port_df.GetAsString());
      state.hidden = !shown.send_position;
    },
    .expert = true,
  });

  AddValue(_("Polar sync"),
           _("Synchronize the glide polar between XCSoar and the device "
             "(LXNAV varios). 'Receive' adopts the polar from the device "
             "(e.g. for club gliders). 'Send' pushes XCSoar's polar to the "
             "device."),
           [this](ValueState &state) {
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             state.hidden = !shown.polar;
             state.text = PolarCaption(ShownPolar(config.polar_sync,
                                                  shown.receive_polar,
                                                  shown.send_polar));
           },
           [this]{
             const auto shown = WhatIsShown(config, port_df.GetAsString());
             std::vector<IdChoice> choices;
             choices.push_back({unsigned(DeviceConfig::PolarSync::OFF),
                                _("Off")});
             if (shown.receive_polar)
               choices.push_back({
                 unsigned(DeviceConfig::PolarSync::RECEIVE),
                 _("Receive from device")});
             if (shown.send_polar)
               choices.push_back({
                 unsigned(DeviceConfig::PolarSync::SEND),
                 _("Send to device")});

             unsigned value = unsigned(ShownPolar(config.polar_sync,
                                                  shown.receive_polar,
                                                  shown.send_polar));
             if (!PickId(_("Polar sync"),
                         _("Synchronize the glide polar between XCSoar and "
                           "the device (LXNAV varios). 'Receive' adopts the "
                           "polar from the device (e.g. for club gliders). "
                           "'Send' pushes XCSoar's polar to the device."),
                         choices, value))
               return;

             config.polar_sync = DeviceConfig::PolarSync(value);
             UpdateValues();
             Notify();
           });

  k6bt_item = GetItemCount();
  AddItem("K6Bt", [this]{
    config.k6bt = IsItemChecked(k6bt_item);
    UpdateVisibilities();
    Notify();
  }, {
    .toggle = true,
    .checked = config.k6bt,
    .help = _("Whether you use a K6Bt to connect the device."),
    .value_callback = [this](ValueState &state) {
      const auto shown = WhatIsShown(config, port_df.GetAsString());
      state.hidden = !shown.k6bt;
    },
    .expert = true,
  });
}

void
DeviceEditWidget::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  Fill();
  GroupedListWidget::Prepare(parent, rc);
  UpdateValues();
}

bool
DeviceEditWidget::Save(bool &_changed) noexcept
{
  DeviceConfig out = baseline;
  bool changed = false;

  changed |= FinishPortField(out, port_df);

  if (baseline.engine_type != DeviceConfig::EngineType::NONE ||
      ShowsEngineType(out.port_type, config.engine_type,
                      out.bluetooth_mac.c_str())) {
    if (out.engine_type != config.engine_type) {
      out.engine_type = config.engine_type;
      changed = true;
    }
  }

  if (out.MaybeBluetooth() && out.k6bt != config.k6bt) {
    out.k6bt = config.k6bt;
    changed = true;
  }

  if (out.UsesSpeed()) {
    if (out.baud_rate != config.baud_rate) {
      out.baud_rate = config.baud_rate;
      changed = true;
    }

    if (out.bulk_baud_rate != config.bulk_baud_rate) {
      out.bulk_baud_rate = config.bulk_baud_rate;
      changed = true;
    }
  }

  if (out.UsesIPAddress())
    changed |= AssignString(out.ip_address, config.ip_address);

  if (out.UsesTCPPort() && out.tcp_port != config.tcp_port) {
    out.tcp_port = config.tcp_port;
    changed = true;
  }

  if (out.port_type == DeviceConfig::PortType::SPECTATE_FILE) {
    changed |= AssignString(out.path, config.path);
    changed |= AssignCallsign(out.port_name, config.port_name);
  }

  if (out.UsesI2C()) {
    if (out.i2c_bus != config.i2c_bus) {
      out.i2c_bus = config.i2c_bus;
      changed = true;
    }

    if (out.i2c_addr != config.i2c_addr) {
      out.i2c_addr = config.i2c_addr;
      changed = true;
    }

    if (out.press_use != config.press_use) {
      out.press_use = config.press_use;
      changed = true;
    }
  }

  if (out.UsesDriver()) {
    if (!StringIsEqual(out.driver_name, config.driver_name)) {
      out.driver_name = config.driver_name;
      changed = true;
    }

    const auto &driver = DriverOf(out.driver_name);
    if (driver.CanReceiveSettings() &&
        out.sync_from_device != config.sync_from_device) {
      out.sync_from_device = config.sync_from_device;
      changed = true;
    }

    if (driver.CanSendSettings() &&
        out.sync_to_device != config.sync_to_device) {
      out.sync_to_device = config.sync_to_device;
      changed = true;
    }

    if (driver.CanSendPosition() &&
        out.send_position != config.send_position) {
      out.send_position = config.send_position;
      changed = true;
    }

    if (driver.CanReceivePolar() || driver.CanSendPolar()) {
      const auto polar = ShownPolar(config.polar_sync,
                                   driver.CanReceivePolar(),
                                   driver.CanSendPolar());
      if (out.polar_sync != polar) {
        out.polar_sync = polar;
        changed = true;
      }
    }

    if (driver.HasPassThrough()) {
      if (out.use_second_device != config.use_second_device) {
        out.use_second_device = config.use_second_device;
        changed = true;
      }

      if (!StringIsEqual(out.driver2_name, config.driver2_name)) {
        out.driver2_name = config.driver2_name;
        changed = true;
      }
    }
  }

  const auto &basic = CommonInterface::Basic();
  if (basic.sensor_calibration_available) {
    out.sensor_offset = basic.sensor_calibration_offset;
    out.sensor_factor = basic.sensor_calibration_factor;
    changed = true;
  }

  config = out;
  _changed |= changed;
  return true;
}
