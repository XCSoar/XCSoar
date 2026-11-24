// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointInfoWidget.hpp"
#include "WaypointTypeName.hpp"
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

/* Same order as Waypoint::Type. */
static constexpr const char *const waypoint_type_names[] = {
  N_("Turnpoint"),
  N_("Airport"),
  N_("Landable"),
  N_("Mountain Top"),
  N_("Mountain Pass"),
  N_("Bridge"),
  N_("Tunnel"),
  N_("Tower"),
  N_("Power Plant"),
  N_("Transmitter Mast"),
  N_("Thermal hotspot"),
  N_("Marker"),
  N_("VOR"),
  N_("NDB"),
  N_("Dam"),
  N_("Castle"),
  N_("Intersection"),
  N_("Control Point"),
  N_("PG Take Off"),
  N_("PG Landing Zone"),
};

static_assert(ARRAY_SIZE(waypoint_type_names) ==
              unsigned(Waypoint::Type::COUNT),
              "waypoint type names must match Waypoint::Type");

const char *
WaypointTypeLabel(Waypoint::Type type) noexcept
{
  const unsigned i = unsigned(type);
  if (i >= unsigned(Waypoint::Type::COUNT))
    return N_("Unknown");
  return waypoint_type_names[i];
}

const char *
GetWaypointTypeName(Waypoint::Type type) noexcept
{
  return gettext(WaypointTypeLabel(type));
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

  if (waypoint->HasElevation())
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
      waypoint->HasElevation()) {
    const GlideState glide_state(basic.location.DistanceBearing(waypoint->location),
                                 waypoint->elevation + settings.task.safety_height_arrival,
                                 basic.nav_altitude,
                                 calculated.GetWindOrZero());

    GlidePolar gp0 = settings.polar.glide_polar_task;
    gp0.SetMC(0);
    AddGlideResult(_("Alt. diff. MC 0"),
                   MacCready::Solve(settings.task.glide,
                                    gp0, glide_state));

    AddGlideResult(_("Alt. diff. Safety MC"),
                   MacCready::Solve(settings.task.glide,
                                    calculated.glide_polar_safety,
                                    glide_state));

    AddGlideResult(_("Alt. diff. Task MC"),
                   MacCready::Solve(settings.task.glide,
                                    settings.polar.glide_polar_task,
                                    glide_state));
  }

  if (basic.location_available && basic.NavAltitudeAvailable() &&
      waypoint->HasElevation()) {
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
