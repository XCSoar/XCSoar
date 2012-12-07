/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "WaypointInfoWidget.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "ComputerSettings.hpp"
#include "Math/SunEphemeris.hpp"
#include "LocalTime.hpp"
#include "Util/StaticString.hpp"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"

static const TCHAR *
FormatGlideResult(TCHAR *buffer, size_t size,
                  const GlideResult &result, const GlideSettings &settings)
{
  switch (result.validity) {
  case GlideResult::Validity::OK:
    FormatRelativeUserAltitude(result.SelectAltitudeDifference(settings),
                               buffer, size);
    return buffer;

  case GlideResult::Validity::WIND_EXCESSIVE:
  case GlideResult::Validity::MACCREADY_INSUFFICIENT:
    return _("Too much wind");

  case GlideResult::Validity::NO_SOLUTION:
    return _("No solution");
  }

  return NULL;
}

void
WaypointInfoWidget::AddGlideResult(const TCHAR *label,
                                   const GlideResult &result)
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  TCHAR buffer[64];
  AddReadOnly(label, NULL, FormatGlideResult(buffer, ARRAY_SIZE(buffer),
                                             result, settings.task.glide));
}

void
WaypointInfoWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  StaticString<64> buffer;

  if (!waypoint.comment.empty())
    AddMultiLine(waypoint.comment.c_str());

  if (waypoint.radio_frequency.IsDefined() &&
      waypoint.radio_frequency.Format(buffer.buffer(),
                                      buffer.MAX_SIZE) != NULL) {
    buffer += _T(" MHz");
    AddReadOnly(_("Radio frequency"), NULL, buffer);
  }

  if (waypoint.runway.IsDirectionDefined())
    buffer.UnsafeFormat(_T("%02u"), waypoint.runway.GetDirectionName());
  else
    buffer.clear();

  if (waypoint.runway.IsLengthDefined()) {
    if (!buffer.empty())
      buffer += _T("; ");

    TCHAR length_buffer[16];
    FormatSmallUserDistance(length_buffer,
                                   fixed(waypoint.runway.GetLength()));
    buffer += length_buffer;
  }

  if (!buffer.empty())
    AddReadOnly(_("Runway"), NULL, buffer);

  if (FormatGeoPoint(waypoint.location,
                     buffer.buffer(), buffer.MAX_SIZE) != NULL)
    AddReadOnly(_("Location"), NULL, buffer);

  FormatUserAltitude(waypoint.elevation,
                            buffer.buffer(), buffer.MAX_SIZE);
  AddReadOnly(_("Elevation"), NULL, buffer);

  if (basic.time_available) {
    const SunEphemeris::Result sun =
      SunEphemeris::CalcSunTimes(waypoint.location, basic.date_time_utc,
                                 fixed(GetUTCOffset()) / 3600);

    const unsigned sunset_hour = (int)sun.time_of_sunset;
    const unsigned sunset_minute = (int)((sun.time_of_sunset - fixed(sunset_hour)) * 60);

    buffer.UnsafeFormat(_T("%02u:%02u"), sunset_hour, sunset_minute);
    AddReadOnly(_("Sunset"), NULL, buffer);
  }

  if (basic.location_available) {
    const GeoVector vector = basic.location.DistanceBearing(waypoint.location);

    TCHAR distance_buffer[32];
    FormatUserDistanceSmart(vector.distance, distance_buffer,
                                   ARRAY_SIZE(distance_buffer));

    FormatBearing(buffer.buffer(), buffer.MAX_SIZE,
                  vector.bearing, distance_buffer);
    AddReadOnly(_("Bearing and Distance"), NULL, buffer);
  }

  if (basic.location_available && basic.NavAltitudeAvailable() &&
      settings.polar.glide_polar_task.IsValid()) {
    const GlideState glide_state(basic.location.DistanceBearing(waypoint.location),
                                 waypoint.elevation + settings.task.safety_height_arrival,
                                 basic.nav_altitude,
                                 calculated.GetWindOrZero());

    GlidePolar gp0 = settings.polar.glide_polar_task;
    gp0.SetMC(fixed(0));
    AddGlideResult(_("Alt. diff. MC 0"),
                   MacCready::Solve(settings.task.glide,
                                    gp0, glide_state));

    AddGlideResult(_("Alt. diff. MC safety"),
                   MacCready::Solve(settings.task.glide,
                                    calculated.glide_polar_safety,
                                    glide_state));

    AddGlideResult(_("Alt. diff. MC current"),
                   MacCready::Solve(settings.task.glide,
                                    settings.polar.glide_polar_task,
                                    glide_state));
  }
}
