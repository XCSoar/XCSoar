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
InfoBoxContentGLoad::Update(InfoBoxData &data)
{
  if (!XCSoarInterface::Basic().acceleration.available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  SetValueFromFixed(data, _T("%2.2f"),
                    XCSoarInterface::Basic().acceleration.g_load);
}

void
InfoBoxContentBattery::Update(InfoBoxData &data)
{
#ifdef HAVE_BATTERY
  bool DisplaySupplyVoltageAsValue=false;
  switch (Power::External::Status) {
    case Power::External::OFF:
      data.SetComment(_("AC Off"));
      break;
    case Power::External::ON:
      if (!XCSoarInterface::Basic().voltage_available)
        data.SetComment(_("AC ON"));
      else{
        DisplaySupplyVoltageAsValue = true;
        SetValueFromFixed(data, _T("%2.1fV"),
                          XCSoarInterface::Basic().voltage);
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
          data.UnsafeFormatValue(_T("%d%%"), Power::Battery::RemainingPercent);
        else
          data.UnsafeFormatComment(_T("%d%%"), Power::Battery::RemainingPercent);
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

  if (XCSoarInterface::Basic().voltage_available) {
    SetValueFromFixed(data, _T("%2.1fV"),
                      XCSoarInterface::Basic().voltage);
    return;
  }

  data.SetInvalid();
}

void
InfoBoxContentExperimental1::Update(InfoBoxData &data)
{
  // Set Value
  data.SetInvalid();
}

void
InfoBoxContentExperimental2::Update(InfoBoxData &data)
{
  // Set Value
  data.SetInvalid();
}

void
InfoBoxContentCPULoad::Update(InfoBoxData &data)
{
  unsigned percent_load = SystemLoadCPU();
  if (percent_load <= 100) {
    data.UnsafeFormatValue(_T("%d%%"), percent_load);
  } else {
    data.SetInvalid();
  }
}

void
InfoBoxContentFreeRAM::Update(InfoBoxData &data)
{
#ifdef HAVE_MEM_INFO
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
  data.UnsafeFormatValue(_T("%.1f%c"), f, unit);
#else
  data.SetInvalid();
#endif
}

void
InfoBoxContentHorizon::on_custom_paint(InfoBoxWindow &infobox, Canvas &canvas)
{
  if (CommonInterface::Basic().acceleration.available) {
    HorizonRenderer::Draw(canvas, infobox.GetValueAndCommentRect(),
                CommonInterface::Basic());
  }
}

void
InfoBoxContentHorizon::Update(InfoBoxData &data)
{
  if (!CommonInterface::Basic().acceleration.available) {
    data.SetInvalid();
    return;
  }

  data.SetCustom();
}
