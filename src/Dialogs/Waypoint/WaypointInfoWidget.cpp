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

#include "WaypointInfoWidget.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/GlideSolvers/GlideState.hpp"
#include "Engine/GlideSolvers/MacCready.hpp"
#include "Engine/GlideSolvers/GlideResult.hpp"
#include "Engine/Util/Gradient.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/Settings.hpp"
#include "Math/SunEphemeris.hpp"
#include "Util/StaticString.hxx"
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

  return nullptr;
}

void
WaypointInfoWidget::AddGlideResult(const TCHAR *label,
                                   const GlideResult &result)
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  TCHAR buffer[64];
  AddReadOnly(label, nullptr,
              FormatGlideResult(buffer, ARRAY_SIZE(buffer),
                                result, settings.task.glide));
}

gcc_const
static BrokenTime
BreakHourOfDay(double t)
{
  /* depending on the time zone, the SunEphemeris library may return a
     negative time of day; the following check catches this before we
     cast the value to "unsigned" */
  if (t < 0)
    t += 24;

  unsigned i = uround(t * 3600);

  BrokenTime result;
  result.hour = i / 3600;
  i %= 3600;
  result.minute = i / 60;
  result.second = i % 60;
  return result;
}

void
WaypointInfoWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  RowFormWidget::Prepare(parent, rc);

  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  StaticString<64> buffer;

  if (!waypoint->comment.empty())
    AddMultiLine(waypoint->comment.c_str());

  if (waypoint->radio_frequency.IsDefined() &&
      waypoint->radio_frequency.Format(buffer.buffer(),
                                      buffer.capacity()) != nullptr) {
    buffer += _T(" MHz");
    AddReadOnly(_("Radio frequency"), nullptr, buffer);
  }

  if (waypoint->runway.IsDirectionDefined())
    buffer.UnsafeFormat(_T("%02u"), waypoint->runway.GetDirectionName());
  else
    buffer.clear();

  if (waypoint->runway.IsLengthDefined()) {
    if (!buffer.empty())
      buffer += _T("; ");

    TCHAR length_buffer[16];
    FormatSmallUserDistance(length_buffer, waypoint->runway.GetLength());
    buffer += length_buffer;
  }

  if (!buffer.empty())
    AddReadOnly(_("Runway"), nullptr, buffer);

  if (FormatGeoPoint(waypoint->location,
                     buffer.buffer(), buffer.capacity()) != nullptr)
    AddReadOnly(_("Location"), nullptr, buffer);

  AddReadOnly(_("Elevation"), nullptr, FormatUserAltitude(waypoint->elevation));

  if (basic.time_available && basic.date_time_utc.IsDatePlausible()) {
    const SunEphemeris::Result sun =
      SunEphemeris::CalcSunTimes(waypoint->location, basic.date_time_utc,
                                 settings.utc_offset);

    const BrokenTime sunrise = BreakHourOfDay(sun.time_of_sunrise);
    const BrokenTime sunset = BreakHourOfDay(sun.time_of_sunset);

    buffer.UnsafeFormat(_T("%02u:%02u - %02u:%02u"),
                        sunrise.hour, sunrise.minute,
                        sunset.hour, sunset.minute);
    AddReadOnly(_("Daylight time"), nullptr, buffer);
  }

  if (basic.location_available) {
    const GeoVector vector = basic.location.DistanceBearing(waypoint->location);

    TCHAR distance_buffer[32];
    FormatUserDistanceSmart(vector.distance, distance_buffer,
                                   ARRAY_SIZE(distance_buffer));

    FormatBearing(buffer.buffer(), buffer.capacity(),
                  vector.bearing, distance_buffer);
    AddReadOnly(_("Bearing and Distance"), nullptr, buffer);
  }

  if (basic.location_available && basic.NavAltitudeAvailable() &&
      settings.polar.glide_polar_task.IsValid()) {
    const GlideState glide_state(basic.location.DistanceBearing(waypoint->location),
                                 waypoint->elevation + settings.task.safety_height_arrival,
                                 basic.nav_altitude,
                                 calculated.GetWindOrZero());

    GlidePolar gp0 = settings.polar.glide_polar_task;
    gp0.SetMC(0);
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

  if (basic.location_available && basic.NavAltitudeAvailable()) {
    const TaskBehaviour &task_behaviour =
      CommonInterface::GetComputerSettings().task;

    const auto safety_height = task_behaviour.safety_height_arrival;
    const auto target_altitude = waypoint->elevation + safety_height;
    const auto delta_h = basic.nav_altitude - target_altitude;
    if (delta_h > 0) {
      const auto distance = basic.location.Distance(waypoint->location);
      const auto gr = distance / delta_h;
      if (GradientValid(gr)) {
        buffer.UnsafeFormat(_T("%.1f"), (double)gr);
        AddReadOnly(_("Required glide ratio"), nullptr, buffer);
      }
    }
  }
}
