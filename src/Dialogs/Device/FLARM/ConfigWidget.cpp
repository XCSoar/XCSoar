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

#include "ConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Traffic.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "system/Sleep.h"

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

[[gnu::pure]]
static bool
SettingExists(FlarmDevice &device, const char *name) noexcept
{
  return (bool)device.GetSetting(name);
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

  try {
    for (auto i = flarm_setting_names; *i != NULL; ++i)
      device.RequestSetting(*i, env);

    for (auto i = flarm_setting_names; *i != NULL; ++i)
      WaitForSetting(device, *i, 500);
  } catch (OperationCancelled) {
    return false;
  } catch (...) {
    env.SetError(std::current_exception());
    return false;
  }

  return true;
}

static unsigned
GetUnsignedValue(const FlarmDevice &device, const char *name,
                 unsigned default_value)
{
  if (const auto x = device.GetSetting(name)) {
    char *endptr;
    unsigned long y = strtoul(x->c_str(), &endptr, 10);
    if (endptr > x->c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}

void
FLARMConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  RequestAllSettings(device);

  baud = GetUnsignedValue(device, "BAUD", 2);
  priv = GetUnsignedValue(device, "PRIV", 0) == 1;
  thre = GetUnsignedValue(device, "THRE", 2);
  range = GetUnsignedValue(device, "RANGE", 3000);
  acft = GetUnsignedValue(device, "ACFT", 0);
  log_int = GetUnsignedValue(device, "LOGINT", 2);
  notrack = GetUnsignedValue(device, "NOTRACK", 0) == 1;

  static constexpr StaticEnumChoice baud_list[] = {
    { 0, _T("4800") },
    { 1, _T("9600") },
    { 2, _T("19200") },
    { 4, _T("38400") },
    { 5, _T("57600") },
    nullptr
  };

  AddEnum(_("Baud rate"), NULL, baud_list, baud);
  AddBoolean(_("Stealth mode"), NULL, priv);
  AddInteger(_("Threshold"), NULL, _T("%d m/s"), _T("%d"), 1, 10, 1, thre);
  AddInteger(_("Range"), NULL, _T("%d m"), _T("%d"), 2000, 25500, 250, range);

  static constexpr StaticEnumChoice acft_list[] = {
    { FlarmTraffic::AircraftType::UNKNOWN, N_("Unknown") },
    { FlarmTraffic::AircraftType::GLIDER, N_("Glider") },
    { FlarmTraffic::AircraftType::TOW_PLANE, N_("Tow plane") },
    { FlarmTraffic::AircraftType::HELICOPTER, N_("Helicopter") },
    { FlarmTraffic::AircraftType::PARACHUTE, N_("Parachute") },
    { FlarmTraffic::AircraftType::DROP_PLANE, N_("Drop plane") },
    { FlarmTraffic::AircraftType::HANG_GLIDER, N_("Hang glider") },
    { FlarmTraffic::AircraftType::PARA_GLIDER, N_("Paraglider") },
    { FlarmTraffic::AircraftType::POWERED_AIRCRAFT, N_("Powered aircraft") },
    { FlarmTraffic::AircraftType::JET_AIRCRAFT, N_("Jet aircraft") },
    { FlarmTraffic::AircraftType::FLYING_SAUCER, N_("Flying saucer") },
    { FlarmTraffic::AircraftType::BALLOON, N_("Balloon") },
    { FlarmTraffic::AircraftType::AIRSHIP, N_("Airship") },
    { FlarmTraffic::AircraftType::UAV, N_("Unmanned aerial vehicle") },
    { FlarmTraffic::AircraftType::STATIC_OBJECT, N_("Static object") },
    nullptr
  };

  AddEnum(_("Type"), NULL, acft_list, acft);
  AddInteger(_("Logger interval"), NULL, _T("%d s"), _T("%d"),
             1, 8, 1, log_int);
  AddBoolean(_("No tracking mode"), NULL, notrack);

}

bool
FLARMConfigWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;
  bool changed = false;
  NarrowString<32> buffer;

  if (SaveValueEnum(Baud, baud)) {
    buffer.UnsafeFormat("%u", baud);
    device.SendSetting("BAUD", buffer, env);
    changed = true;
  }

  if (SaveValue(Priv, priv)) {
    buffer.UnsafeFormat("%u", priv);
    device.SendSetting("PRIV", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(Thre, thre)) {
    buffer.UnsafeFormat("%u", thre);
    device.SendSetting("THRE", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(Range, range)) {
    buffer.UnsafeFormat("%u", range);
    device.SendSetting("RANGE", buffer, env);
    changed = true;
  }

  if (SaveValueEnum(Acft, acft)) {
    buffer.UnsafeFormat("%u", acft);
    device.SendSetting("ACFT", buffer, env);
    changed = true;
  }

  if (SaveValueInteger(LogInt, log_int)) {
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
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), _T("FLARM"));
  return false;
}
