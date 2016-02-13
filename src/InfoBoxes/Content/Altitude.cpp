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

#include "InfoBoxes/Content/Altitude.hpp"
#include "Factory.hpp"
#include "InfoBoxes/Data.hpp"
#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Panel/AltitudeInfo.hpp"
#include "InfoBoxes/Panel/AltitudeSimulator.hpp"
#include "InfoBoxes/Panel/AltitudeSetup.hpp"
#include "Units/Units.hpp"
#include "Interface.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Language/Language.hpp"
#include "Components.hpp"
#include "Simulator.hpp"

#include <tchar.h>

/*
 * Subpart callback function pointers
 */

#ifdef __clang__
/* gcc gives "redeclaration differs in 'constexpr'" */
constexpr
#endif
const InfoBoxPanel altitude_infobox_panels[] = {
  { N_("Simulator"), LoadAltitudeSimulatorPanel },
  { N_("Info"), LoadAltitudeInfoPanel },
  { N_("Setup"), LoadAltitudeSetupPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentAltitude::GetDialogContent() {
  return altitude_infobox_panels;
}

void
UpdateInfoBoxAltitudeNav(InfoBoxData &data)
{
  const MoreData &basic = CommonInterface::Basic();

  if (!basic.NavAltitudeAvailable()) {
    data.SetInvalid();

    if (basic.pressure_altitude_available)
      data.SetComment(_("no QNH"));

    return;
  }

  const ComputerSettings &settings_computer = CommonInterface::GetComputerSettings();

  if (basic.baro_altitude_available &&
      settings_computer.features.nav_baro_altitude_enabled)
    data.SetTitle(InfoBoxFactory::GetCaption(InfoBoxFactory::e_H_Baro));
  else
    data.SetTitle(InfoBoxFactory::GetCaption(InfoBoxFactory::e_HeightGPS));

  data.SetValueFromAltitude(basic.nav_altitude);
  data.SetCommentFromAlternateAltitude(basic.nav_altitude);
}

void
InfoBoxContentAltitudeGPS::Update(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.gps_altitude_available) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(basic.gps_altitude);
  data.SetCommentFromAlternateAltitude(basic.gps_altitude);
}

void
UpdateInfoBoxAltitudeAGL(InfoBoxData &data)
{
  const DerivedInfo &calculated = CommonInterface::Calculated();

  if (!calculated.altitude_agl_valid) {
    data.SetInvalid();
    return;
  }

  data.SetValueFromAltitude(calculated.altitude_agl);
  data.SetCommentFromAlternateAltitude(calculated.altitude_agl);

  // Set Color (red/black)
  data.SetValueColor(calculated.altitude_agl <
      CommonInterface::GetComputerSettings().task.route_planner.safety_height_terrain ? 1 : 0);
}

void
UpdateInfoBoxAltitudeBaro(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.baro_altitude_available) {
    data.SetInvalid();

    if (basic.pressure_altitude_available)
      data.SetComment(_("no QNH"));

    return;
  }

  data.SetValueFromAltitude(basic.baro_altitude);
  data.SetCommentFromAlternateAltitude(basic.baro_altitude);
}

void
UpdateInfoBoxAltitudeQFE(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.gps_altitude_available) {
    data.SetInvalid();
    return;
  }

  auto Value = basic.gps_altitude;

  const auto home_waypoint = way_points.GetHome();
  if (home_waypoint)
    Value -= home_waypoint->elevation;

  data.SetValueFromAltitude(Value);
  data.SetCommentFromAlternateAltitude(Value);
}

void
UpdateInfoBoxAltitudeFlightLevel(InfoBoxData &data)
{
  const NMEAInfo &basic = CommonInterface::Basic();
  const ComputerSettings &settings_computer =
    CommonInterface::GetComputerSettings();

  if (basic.pressure_altitude_available) {
    auto Altitude = Units::ToUserUnit(basic.pressure_altitude, Unit::FEET);

    // Title color black
    data.SetTitleColor(0);

    // Set Value
    data.UnsafeFormatValue(_T("%03d"), iround(Altitude / 100));

    // Set Comment
    data.UnsafeFormatComment(_T("%dft"), iround(Altitude));

  } else if (basic.gps_altitude_available &&
             settings_computer.pressure_available) {
    // Take gps altitude as baro altitude. This is inaccurate but still fits our needs.
    const AtmosphericPressure &qnh = settings_computer.pressure;
    auto Altitude = Units::ToUserUnit(qnh.QNHAltitudeToPressureAltitude(basic.gps_altitude), Unit::FEET);

    // Title color red
    data.SetTitleColor(1);

    // Set Value
    data.UnsafeFormatValue(_T("%03d"), iround(Altitude / 100));

    // Set Comment
    data.UnsafeFormatComment(_T("%dft"), iround(Altitude));

  } else if ((basic.baro_altitude_available || basic.gps_altitude_available) &&
             !settings_computer.pressure_available) {
    data.SetInvalid();
    data.SetComment(_("no QNH"));
  } else {
    data.SetInvalid();
  }
}
