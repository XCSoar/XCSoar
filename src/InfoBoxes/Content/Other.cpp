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

#include "InfoBoxes/Content/Other.hpp"

#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Interface.hpp"
#include "Renderer/HorizonRenderer.hpp"
#include "Hardware/Battery.hpp"
#include "OS/SystemLoad.hpp"
#include "OS/MemInfo.hpp"
#include "Asset.hpp"

#include <tchar.h>
#include <stdio.h>

void
InfoBoxContentGLoad::Update(InfoBoxWindow &infobox)
{
  if (!XCSoarInterface::Basic().acceleration.available) {
    infobox.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(infobox, _T("%2.2f"),
                    XCSoarInterface::Basic().acceleration.g_load);
}

void
InfoBoxContentBattery::Update(InfoBoxWindow &infobox)
{
#ifdef HAVE_BATTERY
  TCHAR tmp[32];

  bool DisplaySupplyVoltageAsValue=false;
  switch (Power::External::Status) {
    case Power::External::OFF:
      infobox.SetComment(_("AC Off"));
      break;
    case Power::External::ON:
      if (!XCSoarInterface::Basic().voltage_available)
        infobox.SetComment(_("AC ON"));
      else{
        DisplaySupplyVoltageAsValue = true;
        SetValueFromFixed(infobox, _T("%2.1fV"),
                          XCSoarInterface::Basic().voltage);
      }
      break;
    case Power::External::UNKNOWN:
    default:
      infobox.SetCommentInvalid();
  }
#ifndef ANDROID
  switch (Power::Battery::Status){
    case Power::Battery::HIGH:
    case Power::Battery::LOW:
    case Power::Battery::CRITICAL:
    case Power::Battery::CHARGING:
      if (Power::Battery::RemainingPercentValid){
#endif
        _stprintf(tmp, _T("%d%%"), Power::Battery::RemainingPercent);
        if (!DisplaySupplyVoltageAsValue)
          infobox.SetValue(tmp);
        else
          infobox.SetComment(tmp);
#ifndef ANDROID
      }
      else
        if (!DisplaySupplyVoltageAsValue)
          infobox.SetValueInvalid();
        else
          infobox.SetCommentInvalid();
      break;
    case Power::Battery::NOBATTERY:
    case Power::Battery::UNKNOWN:
      if (!DisplaySupplyVoltageAsValue)
        infobox.SetValueInvalid();
      else
        infobox.SetCommentInvalid();
  }
#endif
  return;

#endif

  if (XCSoarInterface::Basic().voltage_available) {
    SetValueFromFixed(infobox, _T("%2.1fV"),
                      XCSoarInterface::Basic().voltage);
    return;
  }

  infobox.SetInvalid();
}

void
InfoBoxContentExperimental1::Update(InfoBoxWindow &infobox)
{
  // Set Value
  infobox.SetInvalid();
}

void
InfoBoxContentExperimental2::Update(InfoBoxWindow &infobox)
{
  // Set Value
  infobox.SetInvalid();
}

void
InfoBoxContentCPULoad::Update(InfoBoxWindow &infobox)
{
  TCHAR tmp[32];
  unsigned percent_load = SystemLoadCPU();
  if (percent_load <= 100) {
    _stprintf(tmp, _T("%d%%"), percent_load);
    infobox.SetValue(tmp);
  } else {
    infobox.SetInvalid();
  }
}

void
InfoBoxContentFreeRAM::Update(InfoBoxWindow &infobox)
{
#ifdef HAVE_MEM_INFO
  TCHAR tmp[32];
  TCHAR unit;
  unsigned long freeRAM = SystemFreeRAM();
  double f = freeRAM;
  if (freeRAM >= 1024 * 1024 *1024) {
    f /= (1024 * 1024 * 1024);
    unit = _T('G');
  } else if (freeRAM >= 1024 * 1024) {
    f /= (1024 * 1024);
    unit = _T('M');
  } else if (freeRAM >= 1024) {
    f /= 1024;
    unit = _T('K');
  } else
    unit = _T('B');
  _stprintf(tmp, _T("%.1f%c"), f, unit);
  infobox.SetValue(tmp);
#else
  infobox.SetInvalid();
#endif
}

void
InfoBoxContentHorizon::on_custom_paint(InfoBoxWindow &infobox, Canvas &canvas)
{
  if (CommonInterface::Basic().acceleration.available) {
    DrawHorizon(canvas, infobox.get_value_and_comment_rect(),
                CommonInterface::Basic());
  }
}

void
InfoBoxContentHorizon::Update(InfoBoxWindow &infobox)
{
  infobox.SetComment(_T(""));
  if (!CommonInterface::Basic().acceleration.available) {
    infobox.SetInvalid();
    return;
  }
  infobox.SetValue(_T(""));
  infobox.invalidate();
}
