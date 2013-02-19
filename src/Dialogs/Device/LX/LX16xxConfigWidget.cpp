/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "LX16xxConfigWidget.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Device/Driver/LX/LX1600.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Util/Macros.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "OS/Sleep.h"
#include "Util/NumberParser.hpp"

static constexpr unsigned TIMEOUT = 2000;

static bool
RequestAllSettings(LXDevice &device)
{
  PopupOperationEnvironment env;
  return device.RequestLX16xxSettings(env);
}

static unsigned
WaitUnsignedValue(LXDevice &device, LX1600::Setting key,
                  unsigned default_value)
{
  PopupOperationEnvironment env;
  const auto x = device.WaitLX16xxSetting(key, env, TIMEOUT);
  if (!x.empty()) {
    char *endptr;
    unsigned long y = ParseUnsigned(x.c_str(), &endptr);
    if (endptr > x.c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}

static fixed
WaitFixedValue(LXDevice &device, LX1600::Setting key, fixed default_value)
{
  PopupOperationEnvironment env;
  const auto x = device.WaitLX16xxSetting(key, env, TIMEOUT);
  if (!x.empty()) {
    char *endptr;
    double y = ParseDouble(x.c_str(), &endptr);
    if (endptr > x.c_str() && *endptr == 0)
      return fixed(y);
  }

  return default_value;
}

void
LX16xxConfigWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RequestAllSettings(device);

  AddInteger(_("Avg. time"), NULL, _T("%d s"), _T("%d"), 1, 30, 1,
             WaitUnsignedValue(device, LX1600::Setting::VARIO_AVG_TIME, 25));

  static constexpr StaticEnumChoice range_list[] = {
    { 25, _T("2.5 m/s"), NULL },
    { 50, _T("5.0 m/s"), NULL },
    { 100, _T("10 m/s"), NULL },
    { 0 }
  };

  AddEnum(_("Vario Range"), NULL, range_list,
          (unsigned)(WaitFixedValue(device, LX1600::Setting::VARIO_RANGE,
                                    fixed(5.0)) * 10));
  AddFloat(_("Vario Filter"), NULL,
           _T("%.2f s"), _T("%.2f"), fixed(0.25), fixed(5), fixed(0.25), false,
           WaitFixedValue(device, LX1600::Setting::VARIO_FILTER, fixed(1)));

  static constexpr StaticEnumChoice te_level_list[] = {
    { 0, _T("Off"), NULL },
    { 50, _T("50 %"), NULL },
    { 60, _T("60 %"), NULL },
    { 70, _T("70 %"), NULL },
    { 80, _T("80 %"), NULL },
    { 90, _T("90 %"), NULL },
    { 100, _T("100 %"), NULL },
    { 110, _T("110 %"), NULL },
    { 120, _T("120 %"), NULL },
    { 130, _T("130 %"), NULL },
    { 140, _T("140 %"), NULL },
    { 150, _T("150 %"), NULL },
    { 0 }
  };

  AddEnum(_("TE Level"), NULL, te_level_list,
          (unsigned)WaitFixedValue(device, LX1600::Setting::TE_LEVEL, fixed(50)));

  AddFloat(_("TE Filter"), NULL,
           _T("%.2f s"), _T("%.2f"), fixed(0.0), fixed(2), fixed(1), false,
           WaitFixedValue(device, LX1600::Setting::TE_FILTER, fixed(1.5)));

  AddFloat(_("SMART Filter"), NULL,
           _T("%.2f m/s²"), _T("%.2f"), fixed(0.5), fixed(5), fixed(0.1), false,
           WaitFixedValue(device, LX1600::Setting::SMART_VARIO_FILTER, fixed(1)));
}

bool
LX16xxConfigWidget::SaveFixedSetting(LX1600::Setting key, unsigned idx,
                                     LX1600::SettingsMap &settings)
{
  const std::string old_value = device.GetLX16xxSetting(key);
  fixed value = fixed(ParseDouble(old_value.c_str()));
  if (!SaveValue(idx, value))
    return false;

  NarrowString<32> buffer;
  buffer.UnsafeFormat("%.2f", (double)value);
  settings[key] = std::string(buffer.c_str(), buffer.end());
  return true;
}

bool
LX16xxConfigWidget::SaveUnsignedSetting(LX1600::Setting key, unsigned idx,
                                        LX1600::SettingsMap &settings)
{
  const std::string old_value = device.GetLX16xxSetting(key);
  unsigned value = ParseUnsigned(old_value.c_str());
  if (!SaveValue(idx, value))
    return false;

  NarrowString<32> buffer;
  buffer.UnsafeFormat("%u", value);
  settings[key] = std::string(buffer.c_str(), buffer.end());
  return true;
}

bool
LX16xxConfigWidget::SaveFixedEnumSetting(LX1600::Setting key, unsigned idx,
                                         LX1600::SettingsMap &settings,
                                         unsigned factor)
{
  const std::string old_value = device.GetLX16xxSetting(key);
  unsigned value = unsigned(ParseDouble(old_value.c_str()) * factor);
  if (!SaveValue(idx, value))
    return false;

  NarrowString<32> buffer;
  buffer.UnsafeFormat("%.2f", (double)value / factor);
  settings[key] = std::string(buffer.c_str(), buffer.end());
  return true;
}

bool
LX16xxConfigWidget::Save(bool &changed, bool &require_restart)
{
  LX1600::SettingsMap settings;

  SaveUnsignedSetting(LX1600::Setting::VARIO_AVG_TIME, VARIO_AVG_TIME, settings);
  SaveFixedEnumSetting(LX1600::Setting::VARIO_RANGE, VARIO_RANGE, settings, 10);
  SaveFixedSetting(LX1600::Setting::VARIO_FILTER, VARIO_FILTER, settings);
  SaveUnsignedSetting(LX1600::Setting::TE_LEVEL, TE_LEVEL, settings);
  SaveFixedSetting(LX1600::Setting::TE_FILTER, TE_FILTER, settings);
  SaveFixedSetting(LX1600::Setting::SMART_VARIO_FILTER, SMART_VARIO_FILTER, settings);

  if (!settings.empty()) {
    PopupOperationEnvironment env;
    device.SendLX16xxSettings(settings, env);
    changed = true;
  }

  return true;
}
