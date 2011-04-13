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
#include "Device/Descriptor.hpp"
#include "Asset.hpp"
#include "Protection.hpp"
#include "DevicesConfigPanel.hpp"
#include "Language.hpp"

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
SetupDevice(DeviceDescriptor &device)
{
#ifdef ToDo
  device.DoSetup();
  wf->FocusNext(NULL);
#endif

  // this is a hack, devices dont jet support device dependant setup dialogs

  if (!device.IsVega())
    return;

  changed = dlgConfigurationVarioShowModal();
}


void
DevicesConfigPanel::OnSetupDeviceAClicked(WndButton &button)
{
  SetupDevice(DeviceList[0]);

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  button.set_focus();
}


void
DevicesConfigPanel::OnSetupDeviceBClicked(WndButton &button)
{
  SetupDevice(DeviceList[1]);

  // this is a hack to get the dialog to retain focus because
  // the progress dialog in the vario configuration somehow causes
  // focus problems
  button.set_focus();
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


static void
SetupDeviceFields(const DeviceDescriptor &device, const DeviceConfig &config,
                  WndProperty *port_field, WndProperty *speed_field,
                  WndProperty *driver_field, WndButton *setup_button)
{
#ifndef ANDROID
  static const TCHAR *const COMMPort[] = {
    _T("COM1"), _T("COM2"), _T("COM3"), _T("COM4"),
    _T("COM5"), _T("COM6"), _T("COM7"), _T("COM8"),
    _T("COM9"), _T("COM10"), _T("COM0"),
    NULL
  };

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
    dfe->addEnumTexts(COMMPort);

    switch (config.port_type) {
    case DeviceConfig::SERIAL:
      dfe->Set(config.port_index + num_port_types);
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
    setup_button->set_visible(device.IsVega());
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
        value == config.port_index)
      return false;
#endif

#ifdef ANDROID
    config.port_type = DeviceConfig::RFCOMM;
    config.bluetooth_mac = df.GetAsString();
#else
    config.port_type = DeviceConfig::SERIAL;
    config.port_index = value;
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
  if (speed_field != NULL &&
      (int)config.speed_index != speed_field->GetDataField()->GetAsInteger()) {
    config.speed_index = speed_field->GetDataField()->GetAsInteger();
    changed = true;
  }
#endif

  if (driver_field != NULL &&
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

  SetupDeviceFields(DeviceList[0], device_config[0],
                    (WndProperty*)wf->FindByName(_T("prpComPort1")),
                    (WndProperty*)wf->FindByName(_T("prpComSpeed1")),
                    (WndProperty*)wf->FindByName(_T("prpComDevice1")),
                    (WndButton *)wf->FindByName(_T("cmdSetupDeviceA")));

  SetupDeviceFields(DeviceList[1], device_config[1],
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
DevicesConfigPanel::Save(bool &requirerestart)
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
    if (!is_embedded())
      requirerestart = true;
  }

  return changed;
}
