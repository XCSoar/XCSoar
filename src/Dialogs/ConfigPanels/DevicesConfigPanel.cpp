/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "DataField/Enum.hpp"
#include "DataField/ComboList.hpp"
#include "Form/Edit.hpp"
#include "Form/Util.hpp"
#include "Form/Frame.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Profile/Profile.hpp"
#include "Dialogs/Dialogs.h"
#include "Interface.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Device/Register.hpp"
#include "Device/List.hpp"
#include "Device/Parser.hpp"
#include "Asset.hpp"
#include "Protection.hpp"
#include "DevicesConfigPanel.hpp"
#include "Language/Language.hpp"

#ifdef _WIN32_WCE
#include "Config/Registry.hpp"
#endif

#ifdef ANDROID
#include "Android/BluetoothHelper.hpp"
#endif

static WndForm* wf = NULL;
static bool changed = false;

static const struct {
  enum DeviceConfig::port_type type;
  const TCHAR *label;
} port_types[] = {
  { DeviceConfig::DISABLED, N_("Disabled") },
#ifdef _WIN32_WCE
  { DeviceConfig::AUTO, N_("GPS Intermediate Driver") },
#endif
#ifdef ANDROID
  { DeviceConfig::INTERNAL, N_("Built-in GPS") },
#endif

  /* label not translated for now, until we have a TCP port
     selection UI */
  { DeviceConfig::TCP_LISTENER, _T("TCP port 4353") },

  { DeviceConfig::SERIAL, NULL } /* sentinel */
};

static const unsigned num_port_types =
  sizeof(port_types) / sizeof(port_types[0]) - 1;


static DeviceConfig device_config[NUMDEV];

static void
SetupDevice()
{
#ifdef ToDo
  device.DoSetup();
  wf->FocusNext(NULL);
#endif

  // this is a hack, devices dont jet support device dependant setup dialogs

  changed = dlgConfigurationVarioShowModal();
}


void
DevicesConfigPanel::OnSetupDeviceAClicked(WndButton &button)
{
  SetupDevice();

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  button.set_focus();
}


void
DevicesConfigPanel::OnSetupDeviceBClicked(WndButton &button)
{
  SetupDevice();

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  button.set_focus();
}

static void
UpdateDeviceControlVisibility(WndProperty &port_field,
                              WndProperty &speed_field,
                              WndProperty &driver_field)
{
  unsigned port = port_field.GetDataField()->GetAsInteger();
  enum DeviceConfig::port_type type = port < num_port_types
    ? port_types[port].type
    : (is_android()
       ? DeviceConfig::RFCOMM
       : DeviceConfig::SERIAL);

  speed_field.set_visible(DeviceConfig::UsesSpeed(type));
  driver_field.set_visible(DeviceConfig::UsesDriver(type));
}

void
DevicesConfigPanel::OnDeviceAPort(DataField *Sender,
                                  DataField::DataAccessKind_t Mode)
{
  UpdateDeviceControlVisibility(*(WndProperty *)wf->FindByName(_T("prpComPort1")),
                                *(WndProperty *)wf->FindByName(_T("prpComSpeed1")),
                                *(WndProperty *)wf->FindByName(_T("prpComDevice1")));
}

void
DevicesConfigPanel::OnDeviceBPort(DataField *Sender,
                                  DataField::DataAccessKind_t Mode)
{
  UpdateDeviceControlVisibility(*(WndProperty *)wf->FindByName(_T("prpComPort2")),
                                *(WndProperty *)wf->FindByName(_T("prpComSpeed2")),
                                *(WndProperty *)wf->FindByName(_T("prpComDevice2")));
}

static void
UpdateDeviceSetupButton(unsigned DeviceIdx, const TCHAR *Name)
{
  assert(DeviceIdx < 26);

  TCHAR button_name[] = _T("cmdSetupDeviceA");
  button_name[(sizeof(button_name) / sizeof(button_name[0])) - 2] += DeviceIdx;

  WndButton *wb = (WndButton *)wf->FindByName(button_name);
  if (wb != NULL)
    wb->set_visible(Name != NULL && _tcscmp(Name, _T("Vega")) == 0);
}


void
DevicesConfigPanel::OnDeviceAData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
    UpdateDeviceSetupButton(0, Sender->GetAsString());
    break;

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}


void
DevicesConfigPanel::OnDeviceBData(DataField *Sender, DataField::DataAccessKind_t Mode)
{
  switch (Mode) {
  case DataField::daChange:
    UpdateDeviceSetupButton(1, Sender->GetAsString());
    break;

  case DataField::daInc:
  case DataField::daDec:
  case DataField::daSpecial:
    return;
  }
}

#if defined(HAVE_POSIX) && !defined(ANDROID)

#include <dirent.h>

static bool
DetectSerialPorts(DataFieldEnum &dfe)
{
  DIR *dir = opendir("/dev");
  if (dir == NULL)
    return false;

  unsigned sort_start = dfe.Count();

  bool found = false;
  struct dirent *ent;
  while ((ent = readdir(dir)) != NULL) {
    /* filter "/dev/tty*" */
    if (memcmp(ent->d_name, "tty", 3) != 0)
      continue;

    /* filter out "/dev/tty0", ... (valid integer after "tty") */
    char *endptr;
    strtoul(ent->d_name + 3, &endptr, 10);
    if (*endptr == 0)
      continue;

    char path[64];
    snprintf(path, sizeof(path), "/dev/%s", ent->d_name);
    if (access(path, R_OK|W_OK) == 0 && access(path, X_OK) < 0) {
      dfe.addEnumText(path);
      found = true;
    }
  }

  closedir(dir);

  if (found)
    dfe.Sort(sort_start);

  return found;
}

#endif

#ifdef _WIN32_WCE

gcc_pure
static bool
CompareRegistryValue(const RegistryKey &registry,
                     const TCHAR *name, const TCHAR *value)
{
  TCHAR real_value[64];
  return registry.get_value(name, real_value, 64) &&
    _tcscmp(value, real_value) == 0;
}

gcc_pure
static bool
IsUnimodemPort(const RegistryKey &registry)
{
  return CompareRegistryValue(registry, _T("Tsp"), _T("Unimodem.dll"));
}

gcc_pure
static bool
IsWidcommSerialPort(const RegistryKey &registry)
{
  return CompareRegistryValue(registry, _T("Dll"), _T("btcedrivers.dll")) &&
    CompareRegistryValue(registry, _T("Prefix"), _T("COM"));
}

gcc_pure
static bool
IsSerialPort(const TCHAR *key)
{
  RegistryKey registry(HKEY_LOCAL_MACHINE, key, true);
  if (registry.error())
    return false;

  return IsUnimodemPort(registry) || IsWidcommSerialPort(registry);
}

gcc_pure
static bool
GetDeviceFriendlyName(const TCHAR *key, TCHAR *buffer, size_t max_size)
{
  RegistryKey registry(HKEY_LOCAL_MACHINE, key, true);
  return !registry.error() &&
    registry.get_value(_T("FriendlyName"), buffer, max_size);
}

static bool
DetectSerialPorts(DataFieldEnum &dfe)
{
  RegistryKey drivers_active(HKEY_LOCAL_MACHINE, _T("Drivers\\Active"), true);
  if (drivers_active.error())
    return false;

  unsigned sort_start = dfe.Count();

  bool found = false;
  TCHAR key_name[64], device_key[64], device_name[64];
  for (unsigned i = 0; drivers_active.enum_key(i, key_name, 64); ++i) {
    RegistryKey device(drivers_active, key_name, true);

    if (!device.error() &&
        device.get_value(_T("Key"), device_key, 64) &&
        IsSerialPort(device_key) &&
        device.get_value(_T("Name"), device_name, 64)) {
      TCHAR display_name[256];
      _tcscpy(display_name, device_name);
      size_t length = _tcslen(display_name);
      if (GetDeviceFriendlyName(device_key, display_name + length + 2,
                                256 - length - 3)) {
        /* build a string in the form: "COM1: (Friendly Name)" */
        display_name[length] = _T(' ');
        display_name[length + 1] = _T('(');
        _tcscat(display_name, _T(")"));
      }

      dfe.addEnumText(device_name, display_name);
      found = true;
    }
  }

  if (found)
    dfe.Sort(sort_start);

  return found;
}

#endif

static void
FillPorts(DataFieldEnum &dfe)
{
#if defined(HAVE_POSIX) && !defined(ANDROID)
  if (DetectSerialPorts(dfe))
    return;
#elif defined(WIN32)

#ifdef _WIN32_WCE
  if (DetectSerialPorts(dfe))
    return;
#endif

  for (unsigned i = 1; i <= 10; ++i) {
    TCHAR buffer[64];
    _stprintf(buffer, _T("COM%u:"), i);
    dfe.addEnumText(buffer);
  }
#endif
}

static void
SetupDeviceFields(const DeviceConfig &config,
                  WndProperty *port_field, WndProperty *speed_field,
                  WndProperty *driver_field, WndButton *setup_button)
{
#ifndef ANDROID
  static const TCHAR *const tSpeed[] = {
    _T("1200"), _T("2400"), _T("4800"), _T("9600"),
    _T("19200"), _T("38400"), _T("57600"), _T("115200"),
    NULL
  };
#endif

  if (port_field != NULL) {
    DataFieldEnum *dfe = (DataFieldEnum *)port_field->GetDataField();

    for (unsigned i = 0; port_types[i].label != NULL; i++) {
      dfe->addEnumText(gettext(port_types[i].label));

      if (port_types[i].type == config.port_type)
        dfe->Set(i);
    }

    FillPorts(*dfe);

#ifdef ANDROID
    JNIEnv *env = Java::GetEnv();
    jobjectArray bonded = BluetoothHelper::list(env);
    if (bonded != NULL) {
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

        dfe->addEnumText(address2, name2);
        env->ReleaseStringUTFChars(address, address2);
        if (name2 != NULL)
          env->ReleaseStringUTFChars(name, name2);
      }

      env->DeleteLocalRef(bonded);

      if (config.port_type == DeviceConfig::RFCOMM &&
          !config.bluetooth_mac.empty()) {
        if (!dfe->Exists(config.bluetooth_mac))
          dfe->addEnumText(config.bluetooth_mac);
        dfe->SetAsString(config.bluetooth_mac);
      }
    }
#else
    switch (config.port_type) {
    case DeviceConfig::SERIAL:
      if (!dfe->Exists(config.path))
        dfe->addEnumText(config.path);

      dfe->SetAsString(config.path);
      break;

    case DeviceConfig::DISABLED:
    case DeviceConfig::RFCOMM:
    case DeviceConfig::AUTO:
    case DeviceConfig::INTERNAL:
    case DeviceConfig::TCP_LISTENER:
      break;
    }
#endif

    port_field->RefreshDisplay();
  }

  if (speed_field != NULL) {
#ifdef ANDROID
    speed_field->hide();
#else
    DataFieldEnum *dfe = (DataFieldEnum *)speed_field->GetDataField();
    dfe->addEnumTexts(tSpeed);

    dfe->Set(config.speed_index);
    speed_field->RefreshDisplay();
#endif
  }

  if (driver_field) {
    DataFieldEnum *dfe = (DataFieldEnum *)driver_field->GetDataField();

    const TCHAR *DeviceName;
    for (unsigned i = 0; (DeviceName = devRegisterGetName(i)) != NULL; i++)
      dfe->addEnumText(DeviceName);

    dfe->Sort(1);
    dfe->SetAsString(config.driver_name);

    driver_field->RefreshDisplay();
  }

  if (setup_button != NULL)
    setup_button->set_visible(config.IsVega());

  UpdateDeviceControlVisibility(*port_field, *speed_field, *driver_field);
}


/**
 * @return true if the value has changed
 */
static bool
FinishPortField(DeviceConfig &config, WndProperty &port_field)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)port_field.GetDataField();
  unsigned value = df.GetAsInteger();

  if (value + 1 <= num_port_types) {
    if (port_types[value].type == config.port_type)
      return false;

    config.port_type = port_types[value].type;
    return true;
  } else {
    value -= num_port_types;

#ifdef ANDROID
    if (config.port_type == DeviceConfig::RFCOMM &&
        _tcscmp(config.bluetooth_mac, df.GetAsString()) == 0)
      return false;
#else
    if (config.port_type == DeviceConfig::SERIAL &&
        _tcscmp(config.path, df.GetAsString()) == 0)
      return false;
#endif

#ifdef ANDROID
    config.port_type = DeviceConfig::RFCOMM;
    config.bluetooth_mac = df.GetAsString();
#else
    config.port_type = DeviceConfig::SERIAL;
    config.path = df.GetAsString();
#endif
    return true;
  }
}


static bool
FinishDeviceFields(DeviceConfig &config,
                   WndProperty *port_field, WndProperty *speed_field,
                   WndProperty *driver_field)
{
  bool changed = false;

  if (port_field != NULL && FinishPortField(config, *port_field))
    changed = true;

#ifndef ANDROID
  if (config.UsesSpeed() && speed_field != NULL &&
      (int)config.speed_index != speed_field->GetDataField()->GetAsInteger()) {
    config.speed_index = speed_field->GetDataField()->GetAsInteger();
    changed = true;
  }
#endif

  if (config.UsesDriver() && driver_field != NULL &&
      !config.driver_name.equals(driver_field->GetDataField()->GetAsString())) {
    config.driver_name = driver_field->GetDataField()->GetAsString();
    changed = true;
  }

  return changed;
}


void
DevicesConfigPanel::Init(WndForm *_wf)
{
  assert(_wf != NULL);
  wf = _wf;
  changed = false;

  for (unsigned i = 0; i < NUMDEV; ++i)
    Profile::GetDeviceConfig(i, device_config[i]);

  SetupDeviceFields(device_config[0],
                    (WndProperty*)wf->FindByName(_T("prpComPort1")),
                    (WndProperty*)wf->FindByName(_T("prpComSpeed1")),
                    (WndProperty*)wf->FindByName(_T("prpComDevice1")),
                    (WndButton *)wf->FindByName(_T("cmdSetupDeviceA")));

  SetupDeviceFields(device_config[1],
                    (WndProperty*)wf->FindByName(_T("prpComPort2")),
                    (WndProperty*)wf->FindByName(_T("prpComSpeed2")),
                    (WndProperty*)wf->FindByName(_T("prpComDevice2")),
                    (WndButton *)wf->FindByName(_T("cmdSetupDeviceB")));

  LoadFormProperty(*wf, _T("prpSetSystemTimeFromGPS"),
                   XCSoarInterface::SettingsMap().SetSystemTimeFromGPS);

  LoadFormProperty(*wf, _T("prpIgnoreNMEAChecksum"),
                   NMEAParser::ignore_checksum);
}


bool
DevicesConfigPanel::Save()
{
  DevicePortChanged =
    FinishDeviceFields(device_config[0],
                       (WndProperty*)wf->FindByName(_T("prpComPort1")),
                       (WndProperty*)wf->FindByName(_T("prpComSpeed1")),
                       (WndProperty*)wf->FindByName(_T("prpComDevice1")));

  DevicePortChanged =
    FinishDeviceFields(device_config[1],
                       (WndProperty*)wf->FindByName(_T("prpComPort2")),
                       (WndProperty*)wf->FindByName(_T("prpComSpeed2")),
                       (WndProperty*)wf->FindByName(_T("prpComDevice2"))) ||
    DevicePortChanged;

  changed |= SaveFormProperty(*wf, _T("prpSetSystemTimeFromGPS"),
                              szProfileSetSystemTimeFromGPS,
                              XCSoarInterface::SetSettingsMap().SetSystemTimeFromGPS);

  changed |= SaveFormProperty(*wf, _T("prpIgnoreNMEAChecksum"),
                              szProfileIgnoreNMEAChecksum,
                              NMEAParser::ignore_checksum);

  if (DevicePortChanged) {
    changed = true;
    for (unsigned i = 0; i < NUMDEV; ++i)
      Profile::SetDeviceConfig(i, device_config[i]);
  }

  return changed;
}
