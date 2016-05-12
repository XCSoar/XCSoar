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

#include "InfoBoxes/Content/Other.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "Hardware/Battery.hpp"
#include "OS/SystemLoad.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#include <tchar.h>

void
UpdateInfoBoxGLoad(InfoBoxData &data)
{
  if (!CommonInterface::Basic().acceleration.available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.2f"), CommonInterface::Basic().acceleration.g_load);
}

void
UpdateInfoBoxBattery(InfoBoxData &data)
{
#ifdef HAVE_BATTERY
  bool DisplaySupplyVoltageAsValue=false;
  switch (Power::External::Status) {
    case Power::External::OFF:
      if (CommonInterface::Basic().battery_level_available)
        data.UnsafeFormatComment(_T("%s; %d%%"),
                                 _("AC Off"),
                                 (int)CommonInterface::Basic().battery_level);
      else
        data.SetComment(_("AC Off"));
      break;
    case Power::External::ON:
      if (!CommonInterface::Basic().voltage_available)
        data.SetComment(_("AC ON"));
      else{
        DisplaySupplyVoltageAsValue = true;
        data.SetValueFromVoltage(CommonInterface::Basic().voltage);
      }
      break;
    case Power::External::UNKNOWN:
    default:
      data.SetCommentInvalid();
  }
#ifndef ANDROID
  switch (Power::Battery::Status){
    case Power::Battery::HIGH:
    case Power::Battery::LOW:
    case Power::Battery::CRITICAL:
    case Power::Battery::CHARGING:
      if (Power::Battery::RemainingPercentValid){
#endif
        if (!DisplaySupplyVoltageAsValue)
          data.SetValueFromPercent(Power::Battery::RemainingPercent);
        else
          data.SetCommentFromPercent(Power::Battery::RemainingPercent);
#ifndef ANDROID
      }
      else
        if (!DisplaySupplyVoltageAsValue)
          data.SetValueInvalid();
        else
          data.SetCommentInvalid();
      break;
    case Power::Battery::NOBATTERY:
    case Power::Battery::UNKNOWN:
      if (!DisplaySupplyVoltageAsValue)
        data.SetValueInvalid();
      else
        data.SetCommentInvalid();
  }
#endif
  return;

#endif

  if (CommonInterface::Basic().voltage_available) {
    data.SetValueFromVoltage(CommonInterface::Basic().voltage);
    return;
  } else if (CommonInterface::Basic().battery_level_available) {
    data.SetValueFromPercent(CommonInterface::Basic().battery_level);
    return;
  }

  data.SetInvalid();
}

void
UpdateInfoBoxExperimental1(InfoBoxData &data)
{
  // Set Value
  data.SetInvalid();
}

void
UpdateInfoBoxExperimental2(InfoBoxData &data)
{
  // Set Value
  data.SetInvalid();
}

void
UpdateInfoBoxCPULoad(InfoBoxData &data)
{
  unsigned percent_load = SystemLoadCPU();
  if (percent_load <= 100) {
    data.SetValueFromPercent(percent_load);
  } else {
    data.SetInvalid();
  }
}

void
UpdateInfoBoxFreeRAM(InfoBoxData &data)
{
  // used to be implemented on WinCE
  data.SetInvalid();
}

void
InfoBoxContentHorizon::OnCustomPaint(Canvas &canvas, const PixelRect &rc)
{
  if (CommonInterface::Basic().acceleration.available) {
    const Look &look = UIGlobals::GetLook();
    HorizonRenderer::Draw(canvas, rc,
                          look.horizon, CommonInterface::Basic().attitude);
  }
}

void
InfoBoxContentHorizon::Update(InfoBoxData &data)
{
  if (!CommonInterface::Basic().attitude.IsBankAngleUseable() &&
      !CommonInterface::Basic().attitude.IsPitchAngleUseable()) {
    data.SetInvalid();
    return;
  }

  data.SetCustom();
}
