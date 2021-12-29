/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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
#include "Hardware/PowerGlobal.hpp"
#include "system/SystemLoad.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

#ifdef HAVE_BATTERY
#include "Hardware/PowerInfo.hpp"
#endif

#include <tchar.h>

void
UpdateInfoBoxHeartRate(InfoBoxData &data) noexcept
{
  const auto &basic = CommonInterface::Basic();

  if (!basic.heart_rate_available) {
    data.SetInvalid();
    return;
  }

  data.FormatValue(_T("%u"), basic.heart_rate);
}

void
UpdateInfoBoxGLoad(InfoBoxData &data) noexcept
{
  if (!CommonInterface::Basic().acceleration.available) {
    data.SetInvalid();
    return;
  }

  // Set Value
  data.SetValue(_T("%2.2f"), CommonInterface::Basic().acceleration.g_load);
}

void
UpdateInfoBoxBattery(InfoBoxData &data) noexcept
{
#ifdef HAVE_BATTERY
  const auto &info = Power::global_info;
  const auto &battery = info.battery;
  const auto &external = info.external;

  bool DisplaySupplyVoltageAsValue=false;
  switch (external.status) {
  case Power::ExternalInfo::Status::OFF:
    if (CommonInterface::Basic().battery_level_available)
      data.UnsafeFormatComment(_T("%s; %d%%"),
                               _("AC Off"),
                               (int)CommonInterface::Basic().battery_level);
    else
      data.SetComment(_("AC Off"));
    break;

  case Power::ExternalInfo::Status::ON:
    if (!CommonInterface::Basic().voltage_available)
      data.SetComment(_("AC ON"));
    else{
      DisplaySupplyVoltageAsValue = true;
      data.SetValueFromVoltage(CommonInterface::Basic().voltage);
    }
    break;

  case Power::ExternalInfo::Status::UNKNOWN:
  default:
    data.SetCommentInvalid();
  }

  if (battery.remaining_percent) {
    if (!DisplaySupplyVoltageAsValue)
      data.SetValueFromPercent(*battery.remaining_percent);
    else
      data.SetCommentFromPercent(* battery.remaining_percent);
  } else {
    if (!DisplaySupplyVoltageAsValue)
      data.SetValueInvalid();
    else
      data.SetCommentInvalid();
  }

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
UpdateInfoBoxExperimental1(InfoBoxData &data) noexcept
{
  // Set Value
  data.SetInvalid();
}

void
UpdateInfoBoxExperimental2(InfoBoxData &data) noexcept
{
  // Set Value
  data.SetInvalid();
}

void
UpdateInfoBoxCPULoad(InfoBoxData &data) noexcept
{
  const auto percent_load = SystemLoadCPU();
  if (percent_load) {
    data.SetValueFromPercent(*percent_load);
  } else {
    data.SetInvalid();
  }
}

void
UpdateInfoBoxFreeRAM(InfoBoxData &data) noexcept
{
  // used to be implemented on WinCE
  data.SetInvalid();
}

void
InfoBoxContentHorizon::OnCustomPaint(Canvas &canvas,
                                     const PixelRect &rc) noexcept
{
  if (CommonInterface::Basic().acceleration.available) {
    const Look &look = UIGlobals::GetLook();
    HorizonRenderer::Draw(canvas, rc,
                          look.horizon, CommonInterface::Basic().attitude);
  }
}

void
InfoBoxContentHorizon::Update(InfoBoxData &data) noexcept
{
  const auto &basic = CommonInterface::Basic();

  if (!basic.attitude.IsBankAngleUseable() &&
      !basic.attitude.IsPitchAngleUseable()) {
    data.SetInvalid();
    return;
  }

  /* mix all Validity fields which may have been used to calculate the
     attitude, see ComputeDynamics() */
  data.SetCustom(basic.location_available.ToInteger() +
                 basic.airspeed_available.ToInteger() +
                 basic.attitude.bank_angle_available.ToInteger() +
                 basic.attitude.pitch_angle_available.ToInteger());
}

// TODO: merge with original copy from Dialogs/StatusPanels/SystemStatusPanel.cpp
[[gnu::pure]]
static const TCHAR *
GetGPSStatus(const NMEAInfo &basic) noexcept
{
  if (!basic.alive)
    return N_("Disconnected");
  else if (!basic.location_available)
    return N_("Fix invalid");
  else if (!basic.gps_altitude_available)
    return N_("2D fix");
  else
    return N_("3D fix");
}

void
UpdateInfoBoxNbrSat(InfoBoxData &data) noexcept
{
    const NMEAInfo &basic = CommonInterface::Basic();
    const GPSState &gps = basic.gps;

    data.SetComment(gettext(GetGPSStatus(basic)));

    if (!basic.alive)
        data.SetComment(_("No GPS"));
    else if (gps.satellites_used_available) {
        // known number of sats
        data.FormatValue(_T("%u"), gps.satellites_used);
    } else {
        // valid but unknown number of sats
        data.SetValueInvalid();
    }
}
