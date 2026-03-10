// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "util/StaticString.hxx"
#include "util/Macros.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserGeoPointFormatter.hpp"

static const char *
FormatGlideResult(char *buffer, size_t size,
                  const GlideResult &result,
                  const GlideSettings &settings) noexcept
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

static const char *
GetWaypointTypeName(Waypoint::Type type) noexcept
{
  switch (type) {
  case Waypoint::Type::NORMAL:
    return _("Turnpoint");
  case Waypoint::Type::AIRFIELD:
    return _("Airport");
  case Waypoint::Type::OUTLANDING:
    return _("Landable");
  case Waypoint::Type::MOUNTAIN_PASS:
    return _("Mountain Pass");
  case Waypoint::Type::MOUNTAIN_TOP:
    return _("Mountain Top");
  case Waypoint::Type::OBSTACLE:
    return _("Transmitter Mast");
  case Waypoint::Type::TOWER:
    return _("Tower");
  case Waypoint::Type::TUNNEL:
    return _("Tunnel");
  case Waypoint::Type::BRIDGE:
    return _("Bridge");
  case Waypoint::Type::POWERPLANT:
    return _("Power Plant");
  case Waypoint::Type::VOR:
    return _("VOR");
  case Waypoint::Type::NDB:
    return _("NDB");
  case Waypoint::Type::DAM:
    return _("Dam");
  case Waypoint::Type::CASTLE:
    return _("Castle");
  case Waypoint::Type::INTERSECTION:
    return _("Intersection");
  case Waypoint::Type::MARKER:
    return _("Marker");
  case Waypoint::Type::REPORTING_POINT:
    return _("Control Point");
  case Waypoint::Type::PGTAKEOFF:
    return _("PG Take Off");
  case Waypoint::Type::PGLANDING:
    return _("PG Landing Zone");
  case Waypoint::Type::THERMAL_HOTSPOT:
    return _("Thermal hotspot");
  }

  return _("Unknown");
}

void
WaypointInfoWidget::AddGlideResult(const char *label,
                                   const GlideResult &result) noexcept
{
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  char buffer[64];
  AddReadOnly(label, nullptr,
              FormatGlideResult(buffer, ARRAY_SIZE(buffer),
                                result, settings.task.glide));
}

[[gnu::const]]
static BrokenTime
BreakHourOfDay(double t) noexcept
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
WaypointInfoWidget::Prepare(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const MoreData &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const ComputerSettings &settings = CommonInterface::GetComputerSettings();

  StaticString<64> buffer;

  if (!waypoint->comment.empty())
    AddMultiLine(waypoint->comment.c_str());

  AddReadOnly(_("Type"), nullptr, GetWaypointTypeName(waypoint->type));

  if (!waypoint->shortname.empty())
    AddReadOnly(_("Short Name"), nullptr, waypoint->shortname.c_str());

  if (FormatGeoPoint(waypoint->location,
                     buffer.buffer(), buffer.capacity()) != nullptr)
    AddReadOnly(_("Location"), nullptr, buffer);

  if (waypoint->has_elevation)
    AddReadOnly(_("Elevation"), nullptr, FormatUserAltitude(waypoint->elevation));
  else
    AddReadOnly(_("Elevation"), nullptr, "?");

  if (waypoint->radio_frequency.Format(buffer.buffer(),
                                      buffer.capacity()) != nullptr) {
    buffer += " MHz";
    AddReadOnly(_("Radio frequency"), nullptr, buffer);
  }

  if (waypoint->runway.IsDirectionDefined())
    buffer.UnsafeFormat("%02u", waypoint->runway.GetDirectionName());
  else
    buffer.clear();

  if (waypoint->runway.IsLengthDefined()) {
    if (!buffer.empty())
      buffer += "; ";

    char length_buffer[16];
    FormatSmallUserDistance(length_buffer, waypoint->runway.GetLength());

    if (waypoint->runway.IsWidthDefined()) {
      char width_buffer[16];
      FormatSmallUserDistance(width_buffer, waypoint->runway.GetWidth());

      StaticString<64> runway_size;
      runway_size.UnsafeFormat("%s x %s", length_buffer, width_buffer);
      buffer += runway_size;
    } else {
      buffer += length_buffer;
    }
  }

  if (!buffer.empty())
    AddReadOnly(_("Runway"), nullptr, buffer);

  if (basic.time_available && basic.date_time_utc.IsDatePlausible()) {
    const SunEphemeris::Result sun =
      SunEphemeris::CalcSunTimes(waypoint->location, basic.date_time_utc,
                                 settings.utc_offset);

    const BrokenTime sunrise = BreakHourOfDay(sun.time_of_sunrise);
    const BrokenTime sunset = BreakHourOfDay(sun.time_of_sunset);

    buffer.UnsafeFormat("%02u:%02u - %02u:%02u",
                        sunrise.hour, sunrise.minute,
                        sunset.hour, sunset.minute);
    AddReadOnly(_("Daylight time"), nullptr, buffer);
  }

  if (basic.location_available) {
    const GeoVector vector = basic.location.DistanceBearing(waypoint->location);

    FormatBearing(buffer.buffer(), buffer.capacity(),
                  vector.bearing,
                  FormatUserDistanceSmart(vector.distance));
    AddReadOnly(_("Bearing and Distance"), nullptr, buffer);
  }

  if (basic.location_available && basic.NavAltitudeAvailable() &&
      settings.polar.glide_polar_task.IsValid() &&
      waypoint->has_elevation) {
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

  if (basic.location_available && basic.NavAltitudeAvailable() &&
      waypoint->has_elevation) {
    const TaskBehaviour &task_behaviour =
      CommonInterface::GetComputerSettings().task;

    const auto safety_height = task_behaviour.safety_height_arrival;
    const auto target_altitude = waypoint->elevation + safety_height;
    const auto delta_h = basic.nav_altitude - target_altitude;
    if (delta_h > 0) {
      const auto distance = basic.location.Distance(waypoint->location);
      const auto gr = distance / delta_h;
      if (GradientValid(gr)) {
        buffer.UnsafeFormat("%.1f", (double)gr);
        AddReadOnly(_("Required glide ratio"), nullptr, buffer);
      }
    }
  }
}
