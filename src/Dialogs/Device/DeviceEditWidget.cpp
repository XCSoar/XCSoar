/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#include "DeviceEditWidget.hpp"
#include "UIGlobals.hpp"
#include "Compiler.h"
#include "Util/Macros.hpp"
#include "Util/StringUtil.hpp"
#include "Util/NumberParser.hpp"
#include "Language/Language.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Form/DataField/String.hpp"
#include "Device/Register.hpp"
#include "Device/Driver.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Interface.hpp"

#ifdef _WIN32_WCE
#include "Device/Windows/Enumerator.hpp"
#endif

#ifdef HAVE_POSIX
#include "Device/Port/TTYEnumerator.hpp"
#endif

#ifdef ANDROID
#include "Java/Global.hpp"
#include "Android/BluetoothHelper.hpp"
#include "Device/Port/AndroidIOIOUartPort.hpp"
#endif

enum ControlIndex {
  Port, BaudRate, BulkBaudRate,
  IP_ADDRESS,
  TCPPort,
  PressureType, AnalogBus, I2CSPIBus, I2CBus, I2CAddr, SPIBus, SPIAddr, PressureUsage, Driver,
  SyncFromDevice, SyncToDevice,
  K6Bt,
  IgnoreCheckSum,
};

static constexpr struct {
  DeviceConfig::PortType type;
  const TCHAR *label;
} port_types[] = {
  { DeviceConfig::PortType::DISABLED, N_("Disabled") },
#ifdef _WIN32_WCE
  { DeviceConfig::PortType::AUTO, N_("GPS Intermediate Driver") },
#endif
#if defined(ANDROID) || defined(__APPLE__)
  { DeviceConfig::PortType::INTERNAL, N_("Built-in GPS & sensors") },
#endif
#ifdef ANDROID
  { DeviceConfig::PortType::RFCOMM_SERVER, N_("Bluetooth server") },
  { DeviceConfig::PortType::DROIDSOAR_V2, _T("DroidSoar V2") },
#ifndef NDEBUG
  { DeviceConfig::PortType::NUNCHUCK, N_("IOIO switches and Nunchuk") },
#endif
  { DeviceConfig::PortType::I2CPRESSURESENSOR, N_("IOIO i2c pressure sensor") },
  { DeviceConfig::PortType::IOIOPRESSURE, N_("IOIO pressure sensor") },
  { DeviceConfig::PortType::IOIOVOLTAGE, N_("IOIO voltage sensor") },
#endif

  { DeviceConfig::PortType::TCP_CLIENT, N_("TCP client") },

  /* label not translated for now, until we have a TCP/UDP port
     selection UI */
  { DeviceConfig::PortType::TCP_LISTENER, N_("TCP Port") },
  { DeviceConfig::PortType::UDP_LISTENER, N_("UDP Port") },

  { DeviceConfig::PortType::SERIAL, NULL } /* sentinel */
};

/** the number of fixed port types (excludes Serial, Bluetooth and IOIOUart) */
static constexpr unsigned num_port_types = ARRAY_SIZE(port_types) - 1;

static unsigned
AddPort(DataFieldEnum &df, DeviceConfig::PortType type,
        const TCHAR *text, const TCHAR *display_string=NULL,
        const TCHAR *help=NULL)
{
  /* the uppper 16 bit is the port type, and the lower 16 bit is a
     serial number to make the enum id unique */

  unsigned id = ((unsigned)type << 16) + df.Count();
  df.AddChoice(id, text, display_string, help);
  return id;
}

#if defined(HAVE_POSIX)

#include <dirent.h>
#include <unistd.h>

static bool
DetectSerialPorts(DataFieldEnum &df)
{
  TTYEnumerator enumerator;
  if (enumerator.HasFailed())
    return false;

  unsigned sort_start = df.Count();

  bool found = false;
  const char *path;
  while ((path = enumerator.Next()) != nullptr) {
    const char *display_string = path;
    if (memcmp(path, "/dev/", 5) == 0)
      display_string = path + 5;

    AddPort(df, DeviceConfig::PortType::SERIAL, path, display_string);
    found = true;
  }

  if (found)
    df.Sort(sort_start);

  return found;
}

#endif

#ifdef GNAV

static bool
DetectSerialPorts(DataFieldEnum &df)
{
  AddPort(df, DeviceConfig::PortType::SERIAL, _T("COM1:"), _T("Vario (COM1)"));
  AddPort(df, DeviceConfig::PortType::SERIAL, _T("COM2:"), _T("Radio (COM2)"));
  AddPort(df, DeviceConfig::PortType::SERIAL, _T("COM3:"), _T("Internal (COM3)"));
  return true;
}

#elif defined(_WIN32_WCE)

static bool
DetectSerialPorts(DataFieldEnum &df)
{
  PortEnumerator enumerator;
  if (enumerator.Error())
    return false;

  unsigned sort_start = df.Count();

  bool found = false;
  while (enumerator.Next()) {
    AddPort(df, DeviceConfig::PortType::SERIAL, enumerator.GetName(),
            enumerator.GetDisplayName());
    found = true;
  }

  if (found)
    df.Sort(sort_start);

  return found;
}

#endif

#if defined(WIN32) && !defined(HAVE_POSIX)

static void
FillDefaultSerialPorts(DataFieldEnum &df)
{
  for (unsigned i = 1; i <= 10; ++i) {
    TCHAR buffer[64];
    _stprintf(buffer, _T("COM%u:"), i);
    AddPort(df, DeviceConfig::PortType::SERIAL, buffer);
  }
}

#endif

static void
FillPortTypes(DataFieldEnum &df, const DeviceConfig &config)
{
  for (unsigned i = 0; port_types[i].label != NULL; i++) {
    unsigned id = AddPort(df, port_types[i].type, port_types[i].label,
                          gettext(port_types[i].label));

    if (port_types[i].type == config.port_type)
      df.Set(id);
  }
}

static void
SetPort(DataFieldEnum &df, DeviceConfig::PortType type, const TCHAR *value)
{
  assert(value != NULL);

  if (!df.Set(value))
    df.Set(AddPort(df, type, value));
}

static void
FillSerialPorts(DataFieldEnum &df, const DeviceConfig &config)
{
#if defined(HAVE_POSIX)
  DetectSerialPorts(df);
#elif defined(WIN32)
#ifdef _WIN32_WCE
  if (!DetectSerialPorts(df))
#endif
    FillDefaultSerialPorts(df);
#endif

  if (config.port_type == DeviceConfig::PortType::SERIAL)
    SetPort(df, config.port_type, config.path);
}

static void
FillAndroidBluetoothPorts(DataFieldEnum &df, const DeviceConfig &config)
{
#ifdef ANDROID
  JNIEnv *env = Java::GetEnv();
  jobjectArray bonded = BluetoothHelper::list(env);
  if (bonded == NULL)
    return;

  jsize n = env->GetArrayLength(bonded) / 2;
  for (jsize i = 0; i < n; ++i) {
    jstring address = (jstring)env->GetObjectArrayElement(bonded, i * 2);
    if (address == NULL)
      continue;

    const char *address2 = env->GetStringUTFChars(address, NULL);
    if (address2 == NULL)
      continue;

    jstring name = (jstring)env->GetObjectArrayElement(bonded, i * 2 + 1);
    const char *name2 = name != NULL
      ? env->GetStringUTFChars(name, NULL)
      : NULL;

    AddPort(df, DeviceConfig::PortType::RFCOMM, address2, name2);

    env->ReleaseStringUTFChars(address, address2);
    if (name2 != NULL)
      env->ReleaseStringUTFChars(name, name2);
  }

  env->DeleteLocalRef(bonded);

  if (config.port_type == DeviceConfig::PortType::RFCOMM &&
      !config.bluetooth_mac.empty())
    SetPort(df, DeviceConfig::PortType::RFCOMM, config.bluetooth_mac);
#endif
}

static void
FillAndroidIOIOPorts(DataFieldEnum &df, const DeviceConfig &config)
{
#if defined(ANDROID)
  df.EnableItemHelp(true);

  TCHAR tempID[4];
  TCHAR tempName[15];
  for (unsigned i = 0; i < AndroidIOIOUartPort::getNumberUarts(); i++) {
    StringFormatUnsafe(tempID, _T("%u"), i);
    StringFormat(tempName, sizeof(tempName), _T("IOIO Uart %u"), i);
    unsigned id = AddPort(df, DeviceConfig::PortType::IOIOUART,
                          tempID, tempName,
                          AndroidIOIOUartPort::getPortHelp(i));
    if (config.port_type == DeviceConfig::PortType::IOIOUART &&
        config.ioio_uart_id == i)
      df.Set(id);
  }
#endif
}

static void
FillPorts(DataFieldEnum &df, const DeviceConfig &config)
{
  FillPortTypes(df, config);
  FillSerialPorts(df, config);
  FillAndroidBluetoothPorts(df, config);
  FillAndroidIOIOPorts(df, config);
}

static void
FillBaudRates(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("1200"), 1200);
  dfe.addEnumText(_T("2400"), 2400);
  dfe.addEnumText(_T("4800"), 4800);
  dfe.addEnumText(_T("9600"), 9600);
  dfe.addEnumText(_T("19200"), 19200);
  dfe.addEnumText(_T("38400"), 38400);
  dfe.addEnumText(_T("57600"), 57600);
  dfe.addEnumText(_T("115200"), 115200);
}

static void
FillTCPPorts(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("4353"), 4353);
  dfe.addEnumText(_T("10110"), 10110);
  dfe.addEnumText(_T("4352"), 4352);
}

static void
FillPressType(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("Analog"), (unsigned)DeviceConfig::PressureType::ANALOG);
  dfe.addEnumText(_T("BMP085"), (unsigned)DeviceConfig::PressureType::BMP085);
  dfe.addEnumText(_T("MS5611"), (unsigned)DeviceConfig::PressureType::MS5611);
}

static void
FillAnalogBus(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("0"), 0u);
  dfe.addEnumText(_T("1"), 1u);
  dfe.addEnumText(_T("2"), 2u);
  dfe.addEnumText(_T("3"), 3u);
  dfe.addEnumText(_T("4"), 4u);
  dfe.addEnumText(_T("5"), 5u);
  dfe.addEnumText(_T("6"), 6u);
  dfe.addEnumText(_T("7"), 7u);
  dfe.addEnumText(_T("8"), 8u);
  dfe.addEnumText(_T("9"), 9u);
  dfe.addEnumText(_T("10"), 10u);
  dfe.addEnumText(_T("11"), 11u);
  dfe.addEnumText(_T("12"), 12u);
  dfe.addEnumText(_T("13"), 13u);
  dfe.addEnumText(_T("14"), 14u);
  dfe.addEnumText(_T("15"), 15u);
}

static void
FillI2CBus(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("I2C 0"), 0x00u);
  dfe.addEnumText(_T("I2C 1"), 0x01u);
  dfe.addEnumText(_T("I2C 2"), 0x02u);
}

/* Only lists possible addresses of supported devices */
static void
FillI2CAddr(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("0x76 (MS5611)"), 0x76);
  dfe.addEnumText(_T("0x77 (BMP085 and MS5611)"), 0x77);
//  dfe.addEnumText(_T("0x52 (Nunchuck)"), 0x52); Is implied by device, no choice
//  dfe.addEnumText(_T("0x69 (MPU6050)"), 0x69); Is implied by device, no choice
//  dfe.addEnumText(_T("0x1e (HMC5883)"), 0x1e); Is implied by device, no choice
}

static void
FillSPIBus(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("SPI 0"), 0x20u);
  dfe.addEnumText(_T("SPI 1"), 0x21u);
  dfe.addEnumText(_T("SPI 2"), 0x22u);
}

static void
FillSPIAddr(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("0"), 0u);
}

static void
FillPress(DataFieldEnum &dfe)
{
  dfe.addEnumText(_T("Static & Vario"), (unsigned)DeviceConfig::PressureUse::STATIC_WITH_VARIO);
  dfe.addEnumText(_T("Static"), (unsigned)DeviceConfig::PressureUse::STATIC_ONLY);
  dfe.addEnumText(_T("TE probe (compensated vario)"), (unsigned)DeviceConfig::PressureUse::TEK_PRESSURE);
  dfe.addEnumText(_T("Pitot (airspeed)"), (unsigned)DeviceConfig::PressureUse::PITOT);
  dfe.addEnumText(_T("Pitot zero calibration"), (unsigned)DeviceConfig::PressureUse::PITOT_ZERO);
  dfe.addEnumText(_T("Dynamic (net airspeed)"), (unsigned)DeviceConfig::PressureUse::DYNAMIC);
}

static void
SetPort(DataFieldEnum &df, const DeviceConfig &config)
{
  switch (config.port_type) {
  case DeviceConfig::PortType::DISABLED:
  case DeviceConfig::PortType::AUTO:
  case DeviceConfig::PortType::INTERNAL:
  case DeviceConfig::PortType::DROIDSOAR_V2:
  case DeviceConfig::PortType::NUNCHUCK:
  case DeviceConfig::PortType::I2CPRESSURESENSOR:
  case DeviceConfig::PortType::IOIOPRESSURE:
  case DeviceConfig::PortType::IOIOVOLTAGE:
  case DeviceConfig::PortType::TCP_CLIENT:
  case DeviceConfig::PortType::TCP_LISTENER:
  case DeviceConfig::PortType::UDP_LISTENER:
  case DeviceConfig::PortType::PTY:
  case DeviceConfig::PortType::RFCOMM_SERVER:
    break;

  case DeviceConfig::PortType::SERIAL:
    SetPort(df, config.port_type, config.path);
    return;

  case DeviceConfig::PortType::RFCOMM:
    SetPort(df, config.port_type, config.bluetooth_mac);
    return;

  case DeviceConfig::PortType::IOIOUART:
    StaticString<16> buffer;
    buffer.UnsafeFormat(_T("%d"), config.ioio_uart_id);
    df.Set(buffer);
    return;
  }

  for (unsigned i = 0; port_types[i].label != NULL; i++) {
    if (port_types[i].type == config.port_type) {
      df.Set(port_types[i].label);
      break;
    }
  }
}

DeviceEditWidget::DeviceEditWidget(const DeviceConfig &_config)
  :RowFormWidget(UIGlobals::GetDialogLook()),
   config(_config), listener(NULL) {}


void
DeviceEditWidget::SetConfig(const DeviceConfig &_config)
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

  WndProperty &baud_control = GetControl(BaudRate);
  DataFieldEnum &baud_df = *(DataFieldEnum *)baud_control.GetDataField();
  baud_df.Set(config.baud_rate);
  baud_control.RefreshDisplay();

  WndProperty &bulk_baud_control = GetControl(BulkBaudRate);
  DataFieldEnum &bulk_baud_df = *(DataFieldEnum *)
    bulk_baud_control.GetDataField();
  bulk_baud_df.Set(config.bulk_baud_rate);
  bulk_baud_control.RefreshDisplay();

  WndProperty &ip_address_control = GetControl(IP_ADDRESS);
  DataFieldEnum &ip_address_df = *(DataFieldEnum *)
    ip_address_control.GetDataField();
  ip_address_df.Set(config.ip_address);
  ip_address_control.RefreshDisplay();

  WndProperty &tcp_port_control = GetControl(TCPPort);
  DataFieldEnum &tcp_port_df = *(DataFieldEnum *)
    tcp_port_control.GetDataField();
  tcp_port_df.Set(config.tcp_port);
  tcp_port_control.RefreshDisplay();

  WndProperty &press_type_control = GetControl(PressureType);
  DataFieldEnum &press_type_df = *(DataFieldEnum *)
    press_type_control.GetDataField();
  press_type_df.Set((unsigned)config.press_type);
  press_type_control.RefreshDisplay();

  WndProperty &analog_addr_control = GetControl(AnalogBus);
  DataFieldEnum &analog_addr_df = *(DataFieldEnum *)
    analog_addr_control.GetDataField();
  analog_addr_df.Set(config.i2c_addr);
  analog_addr_control.RefreshDisplay();

  WndProperty &i2cspi_bus_control = GetControl(I2CSPIBus);
  DataFieldEnum &i2cspi_bus_df = *(DataFieldEnum *)
    i2cspi_bus_control.GetDataField();
  i2cspi_bus_df.Set(config.i2c_bus);
  i2cspi_bus_control.RefreshDisplay();

  WndProperty &i2c_bus_control = GetControl(I2CBus);
  DataFieldEnum &i2c_bus_df = *(DataFieldEnum *)
    i2c_bus_control.GetDataField();
  i2c_bus_df.Set(config.i2c_bus);
  i2c_bus_control.RefreshDisplay();

  WndProperty &i2c_addr_control = GetControl(I2CAddr);
  DataFieldEnum &i2c_addr_df = *(DataFieldEnum *)
    i2c_addr_control.GetDataField();
  i2c_addr_df.Set(config.i2c_addr);
  i2c_addr_control.RefreshDisplay();

  WndProperty &spi_bus_control = GetControl(SPIBus);
  DataFieldEnum &spi_bus_df = *(DataFieldEnum *)
    spi_bus_control.GetDataField();
  spi_bus_df.Set(config.i2c_bus);
  spi_bus_control.RefreshDisplay();

  WndProperty &spi_addr_control = GetControl(SPIAddr);
  DataFieldEnum &spi_addr_df = *(DataFieldEnum *)
    spi_addr_control.GetDataField();
  spi_addr_df.Set(config.i2c_addr);
  spi_addr_control.RefreshDisplay();

  WndProperty &press_control = GetControl(PressureUsage);
  DataFieldEnum &press_df = *(DataFieldEnum *)
    press_control.GetDataField();
  press_df.Set((unsigned)config.press_use);
  press_control.RefreshDisplay();

  WndProperty &driver_control = GetControl(Driver);
  DataFieldEnum &driver_df = *(DataFieldEnum *)driver_control.GetDataField();
  driver_df.Set(config.driver_name);
  driver_control.RefreshDisplay();

  WndProperty &sync_from_control = GetControl(SyncFromDevice);
  DataFieldBoolean &sync_from_df =
      *(DataFieldBoolean *)sync_from_control.GetDataField();
  sync_from_df.Set(config.sync_from_device);
  sync_from_control.RefreshDisplay();

  WndProperty &sync_to_control = GetControl(SyncToDevice);
  DataFieldBoolean &sync_to_df =
      *(DataFieldBoolean *)sync_to_control.GetDataField();
  sync_to_df.Set(config.sync_to_device);
  sync_to_control.RefreshDisplay();

  WndProperty &k6bt_control = GetControl(K6Bt);
  DataFieldBoolean &k6bt_df =
      *(DataFieldBoolean *)k6bt_control.GetDataField();
  k6bt_df.Set(config.k6bt);
  k6bt_control.RefreshDisplay();

  WndProperty &ignore_checksum_control = GetControl(IgnoreCheckSum);
  DataFieldBoolean &ignore_checksum_df =
      *(DataFieldBoolean *)ignore_checksum_control.GetDataField();
  ignore_checksum_df.Set(config.ignore_checksum);
  ignore_checksum_control.RefreshDisplay();

  UpdateVisibilities();
}

gcc_pure
static bool
SupportsBulkBaudRate(const DataField &df)
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == NULL)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL)
    return false;

  return driver->SupportsBulkBaudRate();
}

gcc_pure
static bool
CanReceiveSettings(const DataField &df)
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == NULL)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL)
    return false;

  return driver->CanReceiveSettings();
}

gcc_pure
static bool
CanSendSettings(const DataField &df)
{
  const TCHAR *driver_name = df.GetAsString();
  if (driver_name == NULL)
    return false;

  const struct DeviceRegister *driver = FindDriverByName(driver_name);
  if (driver == NULL)
    return false;

  return driver->CanSendSettings();
}

gcc_pure
static DeviceConfig::PortType
GetPortType(const DataField &df)
{
  const DataFieldEnum &dfe = (const DataFieldEnum &)df;
  const unsigned port = dfe.GetValue();

  if (port < num_port_types)
    return port_types[port].type;

  return (DeviceConfig::PortType)(port >> 16);
}

void
DeviceEditWidget::UpdateVisibilities()
{
  const DeviceConfig::PortType type = GetPortType(GetDataField(Port));
  const DeviceConfig::PressureType pressure = (DeviceConfig::PressureType)((const DataFieldEnum &)GetDataField(PressureType)).GetValue();
  const bool maybe_bluetooth =
    DeviceConfig::MaybeBluetooth(type, GetDataField(Port).GetAsString());
  const bool k6bt = maybe_bluetooth && GetValueBoolean(K6Bt);
  const bool uses_speed = DeviceConfig::UsesSpeed(type) || k6bt;

  SetRowAvailable(BaudRate, uses_speed);
  SetRowAvailable(BulkBaudRate, uses_speed &&
                  DeviceConfig::UsesDriver(type));
  SetRowVisible(BulkBaudRate, uses_speed &&
                DeviceConfig::UsesDriver(type) &&
                SupportsBulkBaudRate(GetDataField(Driver)));
  SetRowAvailable(IP_ADDRESS, DeviceConfig::UsesIPAddress(type));
  SetRowAvailable(TCPPort, DeviceConfig::UsesTCPPort(type));
  SetRowAvailable(PressureType, type == DeviceConfig::PortType::IOIOPRESSURE);
  SetRowAvailable(AnalogBus, DeviceConfig::UsesAnalog(type, pressure));
  SetRowAvailable(I2CSPIBus, DeviceConfig::UsesI2C(type, pressure) &&
                  DeviceConfig::UsesSPI(type, pressure));
  SetRowAvailable(I2CBus, DeviceConfig::UsesI2C(type, pressure) &&
                  !DeviceConfig::UsesSPI(type, pressure));
  SetRowAvailable(I2CAddr, DeviceConfig::UsesI2C(type, pressure) &&
                  (!DeviceConfig::UsesSPI(type, pressure) ||
                    (GetDataField(I2CSPIBus).GetAsInteger() & 0x20) != 0x20) &&
                  type != DeviceConfig::PortType::NUNCHUCK);
  SetRowAvailable(SPIBus, DeviceConfig::UsesSPI(type, pressure) &&
                  !DeviceConfig::UsesI2C(type, pressure));
  SetRowAvailable(SPIAddr, DeviceConfig::UsesSPI(type, pressure) &&
                  (!DeviceConfig::UsesI2C(type, pressure) ||
                    (GetDataField(I2CSPIBus).GetAsInteger() & 0x20) == 0x20));
  SetRowAvailable(PressureUsage, DeviceConfig::IsPressureSensor(type));
  SetRowVisible(Driver, DeviceConfig::UsesDriver(type));
  SetRowVisible(SyncFromDevice, DeviceConfig::UsesDriver(type) &&
                CanReceiveSettings(GetDataField(Driver)));
  SetRowVisible(SyncToDevice, DeviceConfig::UsesDriver(type) &&
                CanSendSettings(GetDataField(Driver)));
  SetRowAvailable(K6Bt, maybe_bluetooth);

  SetRowVisible(IgnoreCheckSum, DeviceConfig::UsesDriver(type));
}

void
DeviceEditWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  DataFieldEnum *port_df = new DataFieldEnum(this);
  FillPorts(*port_df, config);
  Add(_("Port"), NULL, port_df);

  DataFieldEnum *baud_rate_df = new DataFieldEnum(this);
  FillBaudRates(*baud_rate_df);
  baud_rate_df->Set(config.baud_rate);
  Add(_("Baud rate"), NULL, baud_rate_df);

  DataFieldEnum *bulk_baud_rate_df = new DataFieldEnum(this);
  bulk_baud_rate_df->addEnumText(_T("Default"), 0u);
  FillBaudRates(*bulk_baud_rate_df);
  bulk_baud_rate_df->Set(config.bulk_baud_rate);
  Add(_("Bulk baud rate"),
      _("The baud rate used for bulk transfers, such as task declaration or flight download."),
      bulk_baud_rate_df);

  DataFieldString *ip_address_df = new DataFieldString(_T(""), this);
  ip_address_df->Set(config.ip_address);
  Add(_("IP Address"), NULL, ip_address_df);

  DataFieldEnum *tcp_port_df = new DataFieldEnum(this);
  FillTCPPorts(*tcp_port_df);
  tcp_port_df->Set(config.tcp_port);
  Add(_("TCP Port"), NULL, tcp_port_df);

  DataFieldEnum *press_type_df = new DataFieldEnum(this);
  FillPressType(*press_type_df);
  press_type_df->Set((unsigned)config.press_type);
  Add(_("Pressure use"), _("Select Type of sensor."),
                           press_type_df);

  DataFieldEnum *analog_bus_df = new DataFieldEnum(this);
  FillAnalogBus(*analog_bus_df);
  analog_bus_df->Set(config.i2c_bus);
  Add(_("Analog Input"), _("Select the sensor input that matches your configuration (0=pin31)."),
      analog_bus_df);

  DataFieldEnum *i2cspi_bus_df = new DataFieldEnum(this);
  FillI2CBus(*i2cspi_bus_df);
  FillSPIBus(*i2cspi_bus_df);
  i2cspi_bus_df->Set(config.i2c_bus);
  Add(_("Digital Bus"), _("The i2c or spi bus number that matches your configuration."), i2cspi_bus_df);

  DataFieldEnum *i2c_bus_df = new DataFieldEnum(this);
  FillI2CBus(*i2c_bus_df);
  i2c_bus_df->Set(config.i2c_bus);
  Add(_("I2C Bus"), _("Select the description or bus number that matches your configuration."), i2c_bus_df);

  DataFieldEnum *i2c_addr_df = new DataFieldEnum(this);
  FillI2CAddr(*i2c_addr_df);
  i2c_addr_df->Set(config.i2c_addr);
  Add(_("I2C Addr"), _("The i2c address that matches your configuration."
                        "This field is not used when your selection in the I2C Bus field is not an i2c bus number. "
                        "In case you do not understand the previous sentence you may assume that this field is not used."),
                        i2c_addr_df);

  DataFieldEnum *spi_bus_df = new DataFieldEnum(this);
  FillSPIBus(*spi_bus_df);
  spi_bus_df->Set(config.i2c_bus);
  Add(_("SPI Bus"), _("The bus number that matches your configuration."), spi_bus_df);

  DataFieldEnum *spi_addr_df = new DataFieldEnum(this);
  FillSPIAddr(*spi_addr_df);
  spi_addr_df->Set(config.i2c_addr);
  Add(_("SPI Addr"), _("The spi address that matches your configuration."), spi_addr_df);

  DataFieldEnum *press_df = new DataFieldEnum(this);
  FillPress(*press_df);
  press_df->Set((unsigned)config.press_use);
  Add(_("Pressure use"), _("Select the purpose of this pressure sensor. "
                           "This sensor measures some pressure. Here you tell the system "
                           "what pressure this is and what its should be used for."),
                           press_df);

  DataFieldEnum *driver_df = new DataFieldEnum(this);

  const struct DeviceRegister *driver;
  for (unsigned i = 0; (driver = GetDriverByIndex(i)) != NULL; i++)
    driver_df->addEnumText(driver->name, driver->display_name);

  driver_df->Sort(1);
  driver_df->Set(config.driver_name);

  Add(_("Driver"), NULL, driver_df);

  AddBoolean(_("Sync. from device"),
             _("This option lets you configure if XCSoar should use settings "
               "like the MacCready value, bugs and ballast from the device."),
             config.sync_from_device, this);
  SetExpertRow(SyncFromDevice);

  AddBoolean(_("Sync. to device"),
             _("This option lets you configure if XCSoar should send settings "
               "like the MacCready value, bugs and ballast to the device."),
             config.sync_to_device, this);
  SetExpertRow(SyncToDevice);

  AddBoolean(_T("K6Bt"),
             _("Enable this if you use a K6Bt to connect the device."),
             config.k6bt, this);
  SetExpertRow(K6Bt);

  AddBoolean(_("Ignore checksum"),
             _("If your GPS device outputs invalid NMEA checksums, this will "
               "allow it's data to be used anyway."),
             config.ignore_checksum, this);
  SetExpertRow(IgnoreCheckSum);

  UpdateVisibilities();
}

/**
 * @return true if the value has changed
 */
static bool
FinishPortField(DeviceConfig &config, const DataFieldEnum &df)
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
  case DeviceConfig::PortType::IOIOPRESSURE:
  case DeviceConfig::PortType::IOIOVOLTAGE:
  case DeviceConfig::PortType::TCP_CLIENT:
  case DeviceConfig::PortType::TCP_LISTENER:
  case DeviceConfig::PortType::UDP_LISTENER:
  case DeviceConfig::PortType::RFCOMM_SERVER:
    if (new_type == config.port_type)
      return false;

    config.port_type = new_type;
    return true;

  case DeviceConfig::PortType::SERIAL:
  case DeviceConfig::PortType::PTY:
    /* Serial Port */
    if (new_type == config.port_type &&
        _tcscmp(config.path, df.GetAsString()) == 0)
      return false;

    config.port_type = new_type;
    config.path = df.GetAsString();
    return true;

  case DeviceConfig::PortType::RFCOMM:
    /* Bluetooth */
    if (new_type == config.port_type &&
        _tcscmp(config.bluetooth_mac, df.GetAsString()) == 0)
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
DeviceEditWidget::Save(bool &_changed)
{
  bool changed = false;

  changed |= FinishPortField(config, (const DataFieldEnum &)GetDataField(Port));

  if (config.MaybeBluetooth())
    changed |= SaveValue(K6Bt, config.k6bt);

  if (config.UsesSpeed()) {
    changed |= SaveValue(BaudRate, config.baud_rate);
    changed |= SaveValue(BulkBaudRate, config.bulk_baud_rate);
  }

  if (config.UsesIPAddress())
    changed |= SaveValue(IP_ADDRESS, config.ip_address.buffer(),
                         config.ip_address.MAX_SIZE);

  if (config.UsesTCPPort())
    changed |= SaveValue(TCPPort, config.tcp_port);

  if (config.UsesI2C() || config.IsPressureSensor()) {
    changed |= SaveValueEnum(PressureType, config.press_type);
    if (config.UsesAnalog())
      changed |= SaveValue(AnalogBus, config.i2c_bus);
    else if (config.UsesI2C() && config.UsesSPI())
    {
      changed |= SaveValue(I2CSPIBus, config.i2c_bus);
      if ((config.i2c_bus & 0x20) == 0x20)
        changed |= SaveValue(SPIAddr, config.i2c_addr);
      else
        changed |= SaveValue(I2CAddr, config.i2c_addr);
    }
    else if (config.UsesI2C())
    {
      changed |= SaveValue(I2CBus, config.i2c_bus);
      changed |= SaveValue(I2CAddr, config.i2c_addr);
    }
    else if (config.UsesSPI())
    {
      changed |= SaveValue(SPIBus, config.i2c_bus);
      changed |= SaveValue(SPIAddr, config.i2c_addr);
    }
    changed |= SaveValueEnum(PressureUsage, config.press_use);
  }

  if (config.UsesDriver()) {
    changed |= SaveValue(Driver, config.driver_name.buffer(),
                         config.driver_name.MAX_SIZE);

    if (CanReceiveSettings(GetDataField(Driver)))
      changed |= SaveValue(SyncFromDevice, config.sync_from_device);

    if (CanSendSettings(GetDataField(Driver)))
      changed |= SaveValue(SyncToDevice, config.sync_to_device);

    changed |= SaveValue(IgnoreCheckSum, config.ignore_checksum);
  }

  if (CommonInterface::Basic().sensor_calibration_available)
    changed = true;

  _changed |= changed;
  return true;
}

void
DeviceEditWidget::OnModified(DataField &df)
{
  if (IsDataField(Port, df) || IsDataField(PressureType, df) || IsDataField(I2CSPIBus, df) || IsDataField(Driver, df) ||
      IsDataField(K6Bt, df))
    UpdateVisibilities();

  if (listener != NULL)
    listener->OnModified(*this);
}
