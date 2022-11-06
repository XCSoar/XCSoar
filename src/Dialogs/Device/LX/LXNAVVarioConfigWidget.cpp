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

#include "LXNAVVarioConfigWidget.hpp"
#include "Dialogs/Error.hpp"
#include "Device/Driver/LX/Internal.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"

static const char *const lxnav_vario_setting_names[] = {
  "BRGPS",
  "BRPDA",
  NULL
};

static bool
RequestAllSettings(LXDevice &device)
{
  PopupOperationEnvironment env;

  for (auto i = lxnav_vario_setting_names; *i != NULL; ++i)
    if (!device.RequestLXNAVVarioSetting(*i, env))
      return false;

  return true;
}

static unsigned
WaitUnsignedValue(LXDevice &device, const char *name,
                  unsigned default_value)
{
  PopupOperationEnvironment env;
  const auto x = device.WaitLXNAVVarioSetting(name, env, 500);
  if (!x.empty()) {
    char *endptr;
    unsigned long y = strtoul(x.c_str(), &endptr, 10);
    if (endptr > x.c_str() && *endptr == 0)
      return (unsigned)y;
  }

  return default_value;
}

void
LXNAVVarioConfigWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  RequestAllSettings(device);

  brgps = WaitUnsignedValue(device, "BRGPS", 5);
  brpda = WaitUnsignedValue(device, "BRPDA", 5);

  static constexpr StaticEnumChoice baud_list[] = {
    { 0, _T("4800") },
    { 1, _T("9600") },
    { 2, _T("19200") },
    { 3, _T("38400") },
    { 4, _T("57600") },
    { 5, _T("115200") },
    { 6, _T("230400") },
    { 7, _T("256000") },
    { 8, _T("460800") },
    { 9, _T("500k") },
    { 10, _T("1M") },
    { 0 }
  };

  AddEnum(_("GPS baud rate"), NULL, baud_list, brgps);
  AddEnum(_("PDA baud rate"), NULL, baud_list, brpda);
}

bool
LXNAVVarioConfigWidget::Save(bool &_changed) noexcept
try {
  PopupOperationEnvironment env;
  bool changed = false;
  NarrowString<32> buffer;

  if (SaveValueEnum(BRGPS, brgps)) {
    buffer.UnsafeFormat("%u", brgps);
    device.SendLXNAVVarioSetting("BRGPS", buffer, env);
    changed = true;
  }

  if (SaveValueEnum(BRPDA, brpda)) {
    buffer.UnsafeFormat("%u", brpda);
    device.SendLXNAVVarioSetting("BRPDA", buffer, env);
    changed = true;
  }

  _changed |= changed;
  return true;
} catch (OperationCancelled) {
  return false;
} catch (...) {
  ShowError(std::current_exception(), _T("Vega"));
  return false;
}
