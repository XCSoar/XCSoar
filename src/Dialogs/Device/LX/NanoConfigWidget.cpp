/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#include "NanoConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"

static const char *const nano_setting_names[] = {
  "BAUDRATE",
  "NMEARATE",
  "AUTOOFF",
  "OFFFIN",
  "NEARDIS",
  "ALWRUN",
  "NMEA",
  "ACCELL",
  "RECINT",
  NULL
};

static bool
RequestAllSettings(LXDevice &device)
{
  PopupOperationEnvironment env;

  for (auto i = nano_setting_names; *i != NULL; ++i)
    if (!device.RequestNanoSetting(*i, env))
      return false;

  return true;
}

static unsigned
WaitUnsignedValue(LXDevice &device, const char *name,
                  unsigned default_value)
{
  PopupOperationEnvironment env;
  const auto x = device.WaitNanoSetting(name, env, 500);
  if (!x.empty()) {
    char *endptr;
    unsigned long y = strtoul(x.c_str(), &endptr, 10);
    if (endptr > x.c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}

static unsigned
WaitBoolValue(LXDevice &device, const char *name,
              bool default_value)
{
  return WaitUnsignedValue(device, name, default_value) != 0;
}

void
NanoConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
  RequestAllSettings(device);

  static constexpr StaticEnumChoice baud_list[] = {
    { 2400, _T("2400") },
    { 4800, _T("4800") },
    { 9600, _T("9600") },
    { 19200, _T("19200") },
    { 38400, _T("38400") },
    { 57600, _T("57600") },
    { 115200, _T("115200") },
    { 0 }
  };

  AddEnum(_("Baud rate"), NULL, baud_list,
          WaitUnsignedValue(device, "BAUDRATE", 115200));

  AddBoolean(_("Auto off"), NULL,
             WaitBoolValue(device, "AUTOOFF", false));

  AddBoolean(_("Auto finish flight"), NULL,
             WaitBoolValue(device, "OFFFIN", true));

  AddBoolean(_("Always run"), NULL,
             WaitBoolValue(device, "ALWRUN", false));

  AddBoolean(_("Enable NMEA"), NULL,
             WaitBoolValue(device, "NMEA", true));

  AddInteger(_("Recording interval"), NULL,
             _T("%d s"), _T("%d"), 1, 60, 1,
             WaitUnsignedValue(device, "RECINT", 1));
}

bool
NanoConfigWidget::SaveSettingBoolean(const char *name, unsigned idx,
                                     OperationEnvironment &env)
{
  bool value = device.GetNanoSettingInteger(name);
  return SaveValue(idx, value) &&
    device.SendNanoSetting(name, value, env);
}

bool
NanoConfigWidget::SaveSettingInteger(const char *name, unsigned idx,
                                     OperationEnvironment &env)
{
  unsigned value = device.GetNanoSettingInteger(name);
  return SaveValueInteger(idx, value) &&
    device.SendNanoSetting(name, value, env);
}

bool
NanoConfigWidget::SaveSettingEnum(const char *name, unsigned idx,
                                  OperationEnvironment &env)
{
  unsigned value = device.GetNanoSettingInteger(name);
  return SaveValueEnum(idx, value) &&
    device.SendNanoSetting(name, value, env);
}

bool
NanoConfigWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;
  bool changed = false;

  changed |= SaveSettingEnum("BAUDRATE", BAUDRATE, env);
  changed |= SaveSettingBoolean("AUTOOFF", AUTOOFF, env);
  changed |= SaveSettingBoolean("OFFFIN", OFFFIN, env);
  changed |= SaveSettingBoolean("ALWRUN", ALWRUN, env);
  changed |= SaveSettingBoolean("NMEA", NMEA, env);
  changed |= SaveSettingInteger("RECINT", RECINT, env);

  _changed |= changed;
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), _T("LXNAV Nano"));
  return false;
}
