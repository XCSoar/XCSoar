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

#include "ConfigWidget.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Traffic.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "OS/Sleep.h"

static const char *const flarm_setting_names[] = {
  "BAUD",
  "PRIV",
  "THRE",
  "RANGE",
  "ACFT",
  "LOGINT",
  "NOTRACK",
  NULL
};

gcc_pure
static bool
SettingExists(FlarmDevice &device, const char *name)
{
  return device.GetSetting(name).first;
}

/**
 * Wait for a setting to be received from the FLARM.
 */
static bool
WaitForSetting(FlarmDevice &device, const char *name, unsigned timeout_ms)
{
  for (unsigned i = 0; i < timeout_ms / 100; ++i) {
    if (SettingExists(device, name))
      return true;
    Sleep(100);
  }

  return false;
}

static bool
RequestAllSettings(FlarmDevice &device)
{
  PopupOperationEnvironment env;

  for (auto i = flarm_setting_names; *i != NULL; ++i)
    if (!device.RequestSetting(*i, env))
      return false;

  for (auto i = flarm_setting_names; *i != NULL; ++i)
    WaitForSetting(device, *i, 500);

  return true;
}

static unsigned
GetUnsignedValue(const FlarmDevice &device, const char *name,
                 unsigned default_value)
{
  auto x = device.GetSetting(name);
  if (x.first) {
    char *endptr;
    unsigned long y = strtoul(x.second.c_str(), &endptr, 10);
    if (endptr > x.second.c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}

void
FLARMConfigWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RequestAllSettings(device);

  baud = GetUnsignedValue(device, "BAUD", 2);
  priv = GetUnsignedValue(device, "PRIV", 0);
  thre = GetUnsignedValue(device, "THRE", 2);
  range = GetUnsignedValue(device, "RANGE", 3000);
  acft = GetUnsignedValue(device, "ACFT", 0);
  log_int = GetUnsignedValue(device, "LOGINT", 2);
  notrack = GetUnsignedValue(device, "NOTRACK", 0);

  static constexpr StaticEnumChoice baud_list[] = {
    { 0, _T("4800"), NULL },
    { 1, _T("9600"), NULL },
    { 2, _T("19200"), NULL },
    { 4, _T("38400"), NULL },
    { 5, _T("57600"), NULL },
    { 0 }
  };

  AddEnum(_("Baud rate"), NULL, baud_list, baud);
  AddBoolean(_("Stealth mode"), NULL, priv == 1);
  AddInteger(_("Threshold"), NULL, _T("%d m/s"), _T("%d"), 1, 10, 1, thre);
  AddInteger(_("Range"), NULL, _T("%d m"), _T("%d"), 2000, 25500, 250, range);

  static constexpr StaticEnumChoice acft_list[] = {
    { (unsigned)FlarmTraffic::AircraftType::UNKNOWN, N_("Unkown") },
    { (unsigned)FlarmTraffic::AircraftType::GLIDER, N_("Glider") },
    { (unsigned)FlarmTraffic::AircraftType::TOW_PLANE, N_("Tow plane") },
    { (unsigned)FlarmTraffic::AircraftType::HELICOPTER, N_("Helicopter") },
    { (unsigned)FlarmTraffic::AircraftType::PARACHUTE, N_("Parachute") },
    { (unsigned)FlarmTraffic::AircraftType::DROP_PLANE, N_("Drop plane") },
    { (unsigned)FlarmTraffic::AircraftType::HANG_GLIDER, N_("Hang glider") },
    { (unsigned)FlarmTraffic::AircraftType::PARA_GLIDER, N_("Paraglider") },
    { (unsigned)FlarmTraffic::AircraftType::POWERED_AIRCRAFT,
      N_("Powered aircraft") },
    { (unsigned)FlarmTraffic::AircraftType::JET_AIRCRAFT, N_("Jet aircraft") },
    { (unsigned)FlarmTraffic::AircraftType::FLYING_SAUCER,
      N_("Flying saucer") },
    { (unsigned)FlarmTraffic::AircraftType::BALLOON, N_("Balloon") },
    { (unsigned)FlarmTraffic::AircraftType::AIRSHIP, N_("Airship") },
    { (unsigned)FlarmTraffic::AircraftType::UAV,
      N_("Unmanned aerial vehicle") },
    { (unsigned)FlarmTraffic::AircraftType::STATIC_OBJECT,
      N_("Static object") },
    { 0 }
  };

  AddEnum(_("Type"), NULL, acft_list, acft);
  AddInteger(_("Logger interval"), NULL, _T("%d s"), _T("%d"),
             1, 8, 1, log_int);
  AddBoolean(_("No tracking mode"), NULL, notrack == 1);

}

bool
FLARMConfigWidget::Save(bool &_changed)
{
  PopupOperationEnvironment env;
  bool changed = false;
  NarrowString<32> buffer;

  if (SaveValue(Baud, baud)) {
    buffer.UnsafeFormat("%u", baud);
    device.SendSetting("BAUD", buffer, env);
    changed = true;
  }

  if (SaveValue(Priv, priv)) {
    buffer.UnsafeFormat("%u", priv);
    device.SendSetting("PRIV", buffer, env);
    changed = true;
  }

  if (SaveValue(Thre, thre)) {
    buffer.UnsafeFormat("%u", thre);
    device.SendSetting("THRE", buffer, env);
    changed = true;
  }

  if (SaveValue(Range, range)) {
    buffer.UnsafeFormat("%u", range);
    device.SendSetting("RANGE", buffer, env);
    changed = true;
  }

  if (SaveValue(Acft, acft)) {
    buffer.UnsafeFormat("%u", acft);
    device.SendSetting("ACFT", buffer, env);
    changed = true;
  }

  if (SaveValue(LogInt, log_int)) {
    buffer.UnsafeFormat("%u", log_int);
    device.SendSetting("LOGINT", buffer, env);
    changed = true;
  }

  if (SaveValue(NoTrack, notrack)) {
    buffer.UnsafeFormat("%u", notrack);
    device.SendSetting("NOTRACK", buffer, env);
    changed = true;
  }

  _changed |= changed;
  return true;
}
