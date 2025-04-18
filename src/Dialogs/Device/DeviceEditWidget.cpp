// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DeviceEditWidget.hpp"
#include "PortDataField.hpp"
#include "PortPicker.hpp"
#include "UIGlobals.hpp"
#include "util/Compiler.h"
#include "util/NumberParser.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/String.hpp"
#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Interface.hpp"

enum ControlIndex {
  Port, EngineTypes, BaudRate, BulkBaudRate,
  IP_ADDRESS,
  TCPPort,
  I2CBus, I2CAddr, PressureUsage, Driver, UseSecondDriver, SecondDriver,
  SyncFromDevice, SyncToDevice,
  K6Bt,
};

static void
FillBaudRates(DataFieldEnum &dfe) noexcept
{
  dfe.addEnumText(_T("1200"), 1200);
  dfe.addEnumText(_T("2400"), 2400);
  dfe.addEnumText(_T("4800"), 4800);
  dfe.addEnumText(_T("9600"), 9600);
  dfe.addEnumText(_T("19200"), 19200);
  dfe.addEnumText(_T("38400"), 38400);
  dfe.addEnumText(_T("57600"), 57600);
  dfe.addEnumText(_T("115200"), 115200);
  dfe.addEnumText(_T("230400"), 230400);
}

static void
FillTCPPorts(DataFieldEnum &dfe) noexcept
{
  dfe.addEnumText(_T("4353"), 4353);
  dfe.addEnumText(_T("10110"), 10110);
  dfe.addEnumText(_T("4352"), 4352);
  dfe.addEnumText(_T("2000"), 2000);
  dfe.addEnumText(_T("23"), 23);
  dfe.addEnumText(_T("8880"), 8880);
  dfe.addEnumText(_T("8881"), 8881);
  dfe.addEnumText(_T("8882"), 8882);
}

static void
FillI2CBus(DataFieldEnum &dfe) noexcept
{
  dfe.addEnumText(_T("0"), 0U);
  dfe.addEnumText(_T("1"), 1U);
  dfe.addEnumText(_T("2"), 2U);
}

/* Only lists possible addresses of supported devices */
static void
FillI2CAddr(DataFieldEnum &dfe) noexcept
{
  dfe.addEnumText(_T("0x76 (MS5611)"), 0x76);
  dfe.addEnumText(_T("0x77 (BMP085 and MS5611)"), 0x77);
//  dfe.addEnumText(_T("0x52 (Nunchuck)"), 0x52); Is implied by device, no choice
//  dfe.addEnumText(_T("0x69 (MPU6050)"), 0x69); Is implied by device, no choice
//  dfe.addEnumText(_T("0x1e (HMC5883)"), 0x1e); Is implied by device, no choice
}

static void
FillPress(DataFieldEnum &dfe) noexcept
{
  dfe.addEnumText(_T("Static & Vario"), (unsigned)DeviceConfig::PressureUse::STATIC_WITH_VARIO);
  dfe.addEnumText(_T("Static"), (unsigned)DeviceConfig::PressureUse::STATIC_ONLY);
  dfe.addEnumText(_T("TE probe (compensated vario)"), (unsigned)DeviceConfig::PressureUse::TEK_PRESSURE);
  dfe.addEnumText(_T("Pitot (airspeed)"), (unsigned)DeviceConfig::PressureUse::PITOT);
}

/**
 * The user can choose from the following engine types:
 * None.
 * 2S1I, 2-stroke one ignition per revolution.
 * 2S2I, 2-stroke two ignitions per revolution.
 * 4S1I, 4-stroke one ignition per revolution.
*/
static void
FillEngineType(DataFieldEnum &dfe) noexcept
{
  dfe.addEnumText(_T("None"), static_cast<unsigned>(DeviceConfig::EngineType::NONE));
  dfe.addEnumText(_T("2S1I"), static_cast<unsigned>(DeviceConfig::EngineType::TWO_STROKE_1_IGN));
  dfe.addEnumText(_T("2S2I"), static_cast<unsigned>(DeviceConfig::EngineType::TWO_STROKE_2_IGN));
  dfe.addEnumText(_T("4S1I"), static_cast<unsigned>(DeviceConfig::EngineType::FOUR_STROKE_1_IGN));
}

static bool
EditPortCallback(const TCHAR *caption, DataField &df,
                 [[maybe_unused]] const TCHAR *help_text) noexcept
{
  return PortPicker((DataFieldEnum &)df, caption);
}

DeviceEditWidget::DeviceEditWidget(const DeviceConfig &_config) noexcept
  :RowFormWidget(UIGlobals::GetDialogLook()),
   config(_config) {}

void
DeviceEditWidget::SetConfig(const DeviceConfig &_config) noexcept
{
  config = _config;

  if (config.port_type == DeviceConfig::PortType::DISABLED)
    /* if the user configures a new device, forget the old "enabled"
       flag and re-enable the device */
    config.enabled = true;

  WndProperty &port_control = GetControl(Port);
  DataFieldEnum &port_df = *(DataFieldEnum *)port_control.GetDataField();
  SetPort(port_df, config);
  port_control.RefreshDisplay();

  LoadValueEnum(BaudRate, config.baud_rate);
  LoadValueEnum(BulkBaudRate, config.bulk_baud_rate);
  LoadValueEnum(IP_ADDRESS, config.ip_address);
  LoadValueEnum(TCPPort, config.tcp_port);
  LoadValueEnum(I2CBus, config.i2c_bus);
  LoadValueEnum(I2CAddr, config.i2c_addr);
  LoadValueEnum(PressureUsage, config.press_use);
  LoadValueEnum(Driver, config.driver_name);
  LoadValue(SyncFromDevice, config.sync_from_device);
  LoadValue(SyncToDevice, config.sync_to_device);
  LoadValue(K6Bt, config.k6bt);
  LoadValueEnum(EngineTypes, config.engine_type);

  UpdateVisibilities();
}

[[gnu::pure]]
static bool
SupportsBulkBaudRate(const DataField &df) noexcept
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == nullptr)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == nullptr)
    return false;

  return driver->SupportsBulkBaudRate();
}

[[gnu::pure]]
static bool
CanReceiveSettings(const DataField &df) noexcept
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == nullptr)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == nullptr)
    return false;

  return driver->CanReceiveSettings();
}

[[gnu::pure]]
static bool
CanSendSettings(const DataField &df) noexcept
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == nullptr)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == nullptr)
    return false;

  return driver->CanSendSettings();
}

[[gnu::pure]]
static bool
CanPassThrough(const DataField &df) noexcept
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == nullptr)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == nullptr)
    return false;

  return driver->HasPassThrough();
}

void
DeviceEditWidget::UpdateVisibilities() noexcept
{
  const auto &port_df = (const DataFieldEnum &)GetDataField(Port);
  const DeviceConfig::PortType type = GetPortType(port_df);
  const bool maybe_bluetooth =
    DeviceConfig::MaybeBluetooth(type, port_df.GetAsString());
  const bool k6bt = maybe_bluetooth && GetValueBoolean(K6Bt);
  const bool uses_speed = DeviceConfig::UsesSpeed(type) || k6bt;
  const bool maybe_engine_sensor = type == DeviceConfig::PortType::BLE_SENSOR;

  SetRowAvailable(BaudRate, uses_speed);
  SetRowAvailable(BulkBaudRate, uses_speed &&
                  DeviceConfig::UsesDriver(type));
  SetRowVisible(BulkBaudRate, uses_speed &&
                DeviceConfig::UsesDriver(type) &&
                SupportsBulkBaudRate(GetDataField(Driver)));
  SetRowAvailable(IP_ADDRESS, DeviceConfig::UsesIPAddress(type));
  SetRowAvailable(TCPPort, DeviceConfig::UsesTCPPort(type));
  SetRowAvailable(I2CBus, DeviceConfig::UsesI2C(type));
  SetRowAvailable(I2CAddr, DeviceConfig::UsesI2C(type) &&
                type != DeviceConfig::PortType::NUNCHUCK);
  SetRowAvailable(PressureUsage, DeviceConfig::IsPressureSensor(type));
  SetRowVisible(Driver, DeviceConfig::UsesDriver(type));

  SetRowVisible(UseSecondDriver, DeviceConfig::UsesDriver(type)
                && CanPassThrough(GetDataField(Driver)));
  SetRowVisible(SecondDriver, DeviceConfig::UsesDriver(type)
                && CanPassThrough(GetDataField(Driver))
                && GetValueBoolean(UseSecondDriver));

  SetRowVisible(SyncFromDevice, DeviceConfig::UsesDriver(type) &&
                CanReceiveSettings(GetDataField(Driver)));
  SetRowVisible(SyncToDevice, DeviceConfig::UsesDriver(type) &&
                CanSendSettings(GetDataField(Driver)));
  SetRowAvailable(K6Bt, maybe_bluetooth);
  SetRowAvailable(EngineTypes, maybe_engine_sensor);
}

void
DeviceEditWidget::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  DataFieldEnum *port_df = new DataFieldEnum(this);
  FillPorts(*port_df, config);
  auto *port_control = Add(_("Port"), nullptr, port_df);
  port_control->SetEditCallback(EditPortCallback);

  DataFieldEnum *engine_type_df = new DataFieldEnum(this);
  FillEngineType(*engine_type_df);
  engine_type_df->SetValue(config.engine_type);
  Add(_("Engine Type"), nullptr, engine_type_df);

  DataFieldEnum *baud_rate_df = new DataFieldEnum(this);
  FillBaudRates(*baud_rate_df);
  baud_rate_df->SetValue(config.baud_rate);
  Add(_("Baud rate"), nullptr, baud_rate_df);

  DataFieldEnum *bulk_baud_rate_df = new DataFieldEnum(this);
  bulk_baud_rate_df->addEnumText(_T("Default"), 0u);
  FillBaudRates(*bulk_baud_rate_df);
  bulk_baud_rate_df->SetValue(config.bulk_baud_rate);
  Add(_("Bulk baud rate"),
      _("The baud rate used for bulk transfers, such as task declaration or flight download."),
      bulk_baud_rate_df);

  DataFieldString *ip_address_df = new DataFieldString(_T(""), this);
  ip_address_df->SetValue(config.ip_address);
  Add(_("IP address"), nullptr, ip_address_df);

  DataFieldEnum *tcp_port_df = new DataFieldEnum(this);
  FillTCPPorts(*tcp_port_df);
  tcp_port_df->SetValue(config.tcp_port);
  Add(_("TCP port"), nullptr, tcp_port_df);

  DataFieldEnum *i2c_bus_df = new DataFieldEnum(this);
  FillI2CBus(*i2c_bus_df);
  i2c_bus_df->SetValue(config.i2c_bus);
  Add(_("I²C bus"), _("Select the description or bus number that matches your configuration."),
                      i2c_bus_df);

  DataFieldEnum *i2c_addr_df = new DataFieldEnum(this);
  FillI2CAddr(*i2c_addr_df);
  i2c_addr_df->SetValue(config.i2c_addr);
  Add(_("I²C addr"), _("The I²C address that matches your configuration. "
                        "This field is not used when your selection in the \"I²C bus\" field is not an I²C bus number. "
                        "Assume this field is not in use if that doesn\'t make sense to you."),
                        i2c_addr_df);

  DataFieldEnum *press_df = new DataFieldEnum(this);
  FillPress(*press_df);
  press_df->SetValue(config.press_use);
  Add(_("Pressure use"), _("Select the purpose of this pressure sensor. "
                           "This sensor measures some pressure. Here you tell the system "
                           "what pressure this is and what it should be used for."),
                           press_df);

  DataFieldEnum *driver_df = new DataFieldEnum(this);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != nullptr; i++)
    driver_df->addEnumText(driver->name, driver->display_name);

  driver_df->Sort(1);
  driver_df->SetValue(config.driver_name);

  Add(_("Driver"), nullptr, driver_df);

  // for a passthrough device, offer additional driver
  AddBoolean(_("Passthrough device"),
             _("Whether the device has a passed-"
               "through device connected."),
             config.use_second_device, this);

  DataFieldEnum *driver2_df = new DataFieldEnum(this);
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != nullptr; i++)
    driver2_df->addEnumText(driver->name, driver->display_name);

  driver2_df->Sort(1);
  driver2_df->SetValue(config.driver2_name);

  Add(_("Second Driver"), nullptr, driver2_df);

  AddBoolean(_("Sync. from device"),
             _("Tells XCSoar to use settings "
               "like the MacCready value, bugs and ballast from the device."),
             config.sync_from_device, this);
  SetExpertRow(SyncFromDevice);

  AddBoolean(_("Sync. to device"),
             _("Tells XCSoar to send settings "
               "like the MacCready value, bugs and ballast to the device."),
             config.sync_to_device, this);
  SetExpertRow(SyncToDevice);

  AddBoolean(_T("K6Bt"),
             _("Whether you use a K6Bt to connect the device."),
             config.k6bt, this);
  SetExpertRow(K6Bt);

  UpdateVisibilities();
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
  case DeviceConfig::PortType::AUTO:
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
    if (new_type == config.port_type)
      return false;

    config.port_type = new_type;
    return true;

  case DeviceConfig::PortType::SERIAL:
  case DeviceConfig::PortType::PTY:
  case DeviceConfig::PortType::ANDROID_USB_SERIAL:
    /* Serial Port */
    if (new_type == config.port_type &&
        StringIsEqual(config.path, df.GetAsString()))
      return false;

    config.port_type = new_type;
    config.path = df.GetAsString();
    return true;

  case DeviceConfig::PortType::RFCOMM:
  case DeviceConfig::PortType::BLE_HM10:
  case DeviceConfig::PortType::BLE_SENSOR:
    /* Bluetooth */
    if (new_type == config.port_type &&
        StringIsEqual(config.bluetooth_mac, df.GetAsString()))
      return false;

    config.port_type = new_type;
    config.bluetooth_mac = df.GetAsString();
    return true;

  case DeviceConfig::PortType::IOIOUART:
    /* IOIO UART */
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

bool
DeviceEditWidget::Save(bool &_changed) noexcept
{
  bool changed = false;

  changed |= FinishPortField(config, (const DataFieldEnum &)GetDataField(Port));

  const bool maybe_engine_sensor = config.port_type == DeviceConfig::PortType::BLE_SENSOR;
  if (maybe_engine_sensor)
    changed |= SaveValueEnum(EngineTypes, config.engine_type);

  if (config.MaybeBluetooth())
    changed |= SaveValue(K6Bt, config.k6bt);

  if (config.UsesSpeed()) {
    changed |= SaveValueEnum(BaudRate, config.baud_rate);
    changed |= SaveValueEnum(BulkBaudRate, config.bulk_baud_rate);
  }

  if (config.UsesIPAddress())
    changed |= SaveValue(IP_ADDRESS, config.ip_address);

  if (config.UsesTCPPort())
    changed |= SaveValueEnum(TCPPort, config.tcp_port);

  if (config.UsesI2C()) {
    changed |= SaveValueEnum(I2CBus, config.i2c_bus);
    changed |= SaveValueEnum(I2CAddr, config.i2c_addr);
    changed |= SaveValueEnum(PressureUsage, config.press_use);
  }

  if (config.UsesDriver()) {
    changed |= SaveValue(Driver, config.driver_name);

    if (CanReceiveSettings(GetDataField(Driver)))
      changed |= SaveValue(SyncFromDevice, config.sync_from_device);

    if (CanSendSettings(GetDataField(Driver)))
      changed |= SaveValue(SyncToDevice, config.sync_to_device);

    if (CanPassThrough(GetDataField(Driver))) {
      changed |= SaveValue(UseSecondDriver, config.use_second_device);
      changed |= SaveValue(SecondDriver, config.driver2_name.buffer(),
                           config.driver2_name.capacity());
    }
  }

  if (CommonInterface::Basic().sensor_calibration_available)
    changed = true;

  _changed |= changed;
  return true;
}

void
DeviceEditWidget::OnModified(DataField &df) noexcept
{
  if (IsDataField(Port, df) || IsDataField(Driver, df) ||
      IsDataField(UseSecondDriver, df) || IsDataField(K6Bt, df))
    UpdateVisibilities();

  if (listener != nullptr)
    listener->OnModified(*this);
}
