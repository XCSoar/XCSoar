// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CupTaskQR.hpp"

#include "Engine/Task/Factory/TaskFactoryType.hpp"
#include "Engine/Task/ObservationZones/AnnularSectorZone.hpp"
#include "Engine/Task/ObservationZones/KeyholeZone.hpp"
#include "Engine/Task/ObservationZones/LineSectorZone.hpp"
#include "Engine/Task/ObservationZones/SymmetricSectorZone.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Settings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Geo/AltitudeReference.hpp"
#include "Math/Angle.hpp"
#include "Task/TaskFileSeeYou.hpp"

#include <cmath>
#include <fmt/format.h>
#include <stdexcept>

/* TaskFileSeeYou splits one task line into 30 columns.  Three of
   those are the task name, the takeoff and the landing. */
static constexpr unsigned CUP_MAX_TASK_POINTS = 27;

static bool
Near(double value, double expect, double tolerance = 0.5) noexcept
{
  return std::fabs(value - expect) < tolerance;
}

static void
AppendQuoted(std::string &out, std::string_view value)
{
  out.push_back('"');
  for (const char ch : value) {
    if (ch == '"') out.push_back('"');
    out.push_back(ch);
  }
  out.push_back('"');
}

static void
AppendDMM(std::string &out, const Angle angle, bool latitude)
{
  const auto dmm = angle.ToDMM();
  out += fmt::format("{:0{}}{:02}.{:03}{}",
                     dmm.degrees,
                     latitude ? 2 : 3,
                     dmm.minutes,
                     dmm.decimal_minutes,
                     latitude ? (dmm.positive ? 'N' : 'S')
                              : (dmm.positive ? 'E' : 'W'));
}

static void
AppendMetres(std::string &out, double metres)
{
  const double rounded = std::round(metres);
  if (Near(metres, rounded, 0.05))
    out += fmt::format("{:.0f}m", rounded);
  else
    out += fmt::format("{:.1f}m", metres);
}

static void
AppendDegrees(std::string &out, double degrees)
{
  const double rounded = std::round(degrees);
  if (Near(degrees, rounded, 0.05))
    out += fmt::format("{:.0f}", rounded);
  else
    out += fmt::format("{:.2f}", degrees);
}

/**
 * A 90 degree sector of at least 3 km is what SeeYou writes for an
 * FAI quadrant, and what TaskFileSeeYou turns back into one.
 */
static bool
IsFaiQuadrant(double radius, double full_angle_deg) noexcept
{
  return radius >= 3000 && Near(full_angle_deg, 90, 1);
}

static void
AppendSymmetric(std::string &out, double radius, double full_angle_deg)
{
  out += ",Style=1,R1=";
  AppendMetres(out, radius);
  out += ",A1=";
  AppendDegrees(out, full_angle_deg / 2);
}

static bool
AppendFaiOrSymmetric(std::string &out,
                     double radius,
                     double full_angle_deg,
                     bool intermediate)
{
  const double fai_radius = intermediate ? 10000 : 1000;
  if (Near(radius, fai_radius) && Near(full_angle_deg, 90, 1)) {
    /* No radius: the reader builds its own FAI sector. */
    out += ",Style=1";
    return true;
  }

  if (IsFaiQuadrant(radius, full_angle_deg)) return false;

  if (!(radius > 0) || !(full_angle_deg > 0)) return false;

  AppendSymmetric(out, radius, full_angle_deg);
  return true;
}

static bool
AppendBgaFixed(std::string &out, const KeyholeZone &zone, bool intermediate)
{
  if (!intermediate || !Near(zone.GetRadius(), 20000) ||
      !Near(zone.GetSectorAngle().Degrees(), 90) ||
      !Near(zone.GetInnerRadius(), 500))
    return false;

  out += ",Style=1,R1=20000m,A1=45,R2=500m,A2=180";
  return true;
}

static bool
AppendBgaEnhanced(std::string &out, const KeyholeZone &zone, bool intermediate)
{
  if (!intermediate || !Near(zone.GetRadius(), 10000) ||
      !Near(zone.GetSectorAngle().Degrees(), 180) ||
      !Near(zone.GetInnerRadius(), 500))
    return false;

  out += ",Style=1,R1=10000m,A1=90,R2=500m,A2=180";
  return true;
}

static bool
AppendSector(std::string &out, const SectorZone &zone, bool annular)
{
  const Angle sweep =
    (zone.GetEndRadial() - zone.GetStartRadial()).AsBearing();
  const double sweep_deg = sweep.Degrees();
  if (sweep_deg < 1 || sweep_deg > 359) return false;

  const Angle half   = sweep.Half();
  const Angle centre = (zone.GetStartRadial() + half).AsBearing();
  if (!(zone.GetRadius() > 0)) return false;

  out += ",Style=0,R1=";
  AppendMetres(out, zone.GetRadius());
  out += ",A1=";
  AppendDegrees(out, half.Degrees());
  out += ",A12=";
  AppendDegrees(out, centre.Reciprocal().AsBearing().Degrees());

  if (!annular) {
    /* An unset inner radius still defaults to 200 m and would make
       this an annular sector.  A2 keeps it a plain sector. */
    out += ",A2=180";
    return true;
  }

  const auto &annulus = static_cast<const AnnularSectorZone &>(zone);
  if (!(annulus.GetInnerRadius() > 0)) return false;

  out += ",R2=";
  AppendMetres(out, annulus.GetInnerRadius());
  out += ",A2=0";
  return true;
}

static std::optional<std::string>
FormatObsZone(unsigned index,
              const ObservationZonePoint &oz,
              bool intermediate,
              TaskFactoryType factory,
              unsigned start_max_msl)
{
  std::string line = fmt::format("ObsZone={}", index);
  const auto shape = oz.GetShape();
  bool ok          = false;

  switch (shape) {
  case ObservationZone::Shape::LINE:
    if (!intermediate) {
      const auto &line_zone = static_cast<const LineSectorZone &>(oz);
      const double half     = line_zone.GetLength() / 2;
      if (half > 0) {
        line += ",Style=1,R1=";
        AppendMetres(line, half);
        line += ",Line=1";
        ok = true;
      }
    }
    break;

  case ObservationZone::Shape::CYLINDER:
  case ObservationZone::Shape::MAT_CYLINDER:
    if (static_cast<const CylinderZone &>(oz).GetRadius() > 0) {
      line += ",Style=0,R1=";
      AppendMetres(line, static_cast<const CylinderZone &>(oz).GetRadius());
      line += ",A1=180";
      ok = true;
    }
    break;

  case ObservationZone::Shape::FAI_SECTOR:
  case ObservationZone::Shape::SYMMETRIC_SECTOR: {
    const auto &sector = static_cast<const SymmetricSectorZone &>(oz);
    ok                 = AppendFaiOrSymmetric(line,
                              sector.GetRadius(),
                              sector.GetSectorAngle().Degrees(),
                              intermediate);
    break;
  }

  case ObservationZone::Shape::BGAFIXEDCOURSE:
    ok =
      AppendBgaFixed(line, static_cast<const KeyholeZone &>(oz), intermediate);
    break;

  case ObservationZone::Shape::BGAENHANCEDOPTION:
    ok = AppendBgaEnhanced(
      line, static_cast<const KeyholeZone &>(oz), intermediate);
    break;

  case ObservationZone::Shape::CUSTOM_KEYHOLE: {
    const auto &keyhole = static_cast<const KeyholeZone &>(oz);
    ok                  = AppendBgaFixed(line, keyhole, intermediate) ||
         AppendBgaEnhanced(line, keyhole, intermediate);
    break;
  }

  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::ANNULAR_SECTOR:
    if (factory == TaskFactoryType::AAT && intermediate)
      ok = AppendSector(line,
                        static_cast<const SectorZone &>(oz),
                        shape == ObservationZone::Shape::ANNULAR_SECTOR);
    break;

  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::BGA_START:
    break;
  }

  if (!ok) return std::nullopt;

  if (index == 0 && start_max_msl > 0) {
    line += ",MaxAlt=";
    AppendMetres(line, start_max_msl);
  }

  line.push_back('\n');
  return line;
}

struct CupPoint
{
  std::string label;
  GeoPoint location;
};

/**
 * @return the CUP name for this waypoint, or an empty string when
 * every suffix is already used by a different place
 */
static std::string
LabelFor(const Waypoint &wp, const CupPoint previous[], unsigned count)
{
  const std::string base = wp.name.empty() ? "TP" : wp.name;
  std::string candidate  = base;

  for (unsigned n = 2; n < 100; ++n) {
    const CupPoint *match = nullptr;
    for (unsigned i = 0; i < count; ++i)
      if (previous[i].label == candidate) {
        match = &previous[i];
        break;
      }

    /* A new name, or the same name at the same place.  SeeYou looks
       the task column up by name, so two places cannot share one. */
    if (match == nullptr || match->location.DistanceS(wp.location) <= 10)
      return candidate;

    candidate = fmt::format("{} {}", base, n);
  }

  return {};
}

std::optional<std::string>
EncodeCupTaskQRText(const OrderedTask &task)
try {
  const unsigned n = task.TaskSize();
  if (n < 2 || n > CUP_MAX_TASK_POINTS) return std::nullopt;

  if (task.GetOptionalStartPointCount() > 0) return std::nullopt;

  const TaskFactoryType factory = task.GetFactoryType();
  if (factory != TaskFactoryType::RACING && factory != TaskFactoryType::AAT)
    return std::nullopt;

  const auto &settings = task.GetOrderedTaskSettings();
  if (settings.finish_constraints.min_height > 0) return std::nullopt;

  const auto &start = settings.start_constraints;
  if (start.max_height > 0 && start.max_height_ref != AltitudeReference::MSL)
    return std::nullopt;

  if (factory == TaskFactoryType::AAT &&
      settings.aat_min_time.count() >= 100u * 3600u)
    return std::nullopt;

  const unsigned start_max_msl =
    start.max_height_ref == AltitudeReference::MSL ? start.max_height : 0;

  CupPoint points[CUP_MAX_TASK_POINTS];
  for (unsigned i = 0; i < n; ++i) {
    const auto &wp  = task.GetPoint(i).GetWaypoint();
    points[i].label = LabelFor(wp, points, i);
    if (points[i].label.empty()) return std::nullopt;
    points[i].location = wp.location;
  }

  std::string zones;
  for (unsigned i = 0; i < n; ++i) {
    const bool intermediate = i > 0 && i + 1 < n;
    auto line               = FormatObsZone(i,
                              task.GetPoint(i).GetObservationZone(),
                              intermediate,
                              factory,
                              start_max_msl);
    if (!line) return std::nullopt;
    zones += *line;
  }

  std::string text{CUP_TASK_QR_PREFIX};
  text += "name,code,country,lat,lon,elev,style,"
          "rwdir,rwlen,rwwidth,freq,desc\n";

  for (unsigned i = 0; i < n; ++i) {
    bool seen = false;
    for (unsigned j = 0; j < i; ++j)
      if (points[j].label == points[i].label) {
        seen = true;
        break;
      }
    if (seen) continue;

    const auto &wp = task.GetPoint(i).GetWaypoint();
    AppendQuoted(text, points[i].label);
    text += ",\"\",,";
    AppendDMM(text, wp.location.latitude, true);
    text.push_back(',');
    AppendDMM(text, wp.location.longitude, false);
    text.push_back(',');
    if (wp.HasElevation()) AppendMetres(text, wp.elevation);
    text += ",1\n";
  }

  text += "-----Related Tasks-----\n";
  AppendQuoted(text, task.GetName().c_str());
  text.push_back(',');
  /* Takeoff, then the task points, then landing.  The reader drops
     the takeoff and the landing. */
  AppendQuoted(text, points[0].label);
  for (unsigned i = 0; i < n; ++i) {
    text.push_back(',');
    AppendQuoted(text, points[i].label);
  }
  text.push_back(',');
  AppendQuoted(text, points[n - 1].label);
  text.push_back('\n');

  if (factory == TaskFactoryType::AAT) {
    const unsigned seconds = settings.aat_min_time.count();
    text += fmt::format("Options,WpDis=False,TaskTime="
                        "{:02}:{:02}:{:02}\n",
                        seconds / 3600,
                        (seconds / 60) % 60,
                        seconds % 60);
  } else
    text += "Options,WpDis=True\n";

  text += zones;
  return text;
} catch (...) {
  return std::nullopt;
}

std::unique_ptr<OrderedTask>
DecodeCupTaskQRText(std::string_view cup, const TaskBehaviour &task_behaviour)
{
  auto task = LoadSeeYouTaskText(cup, task_behaviour);
  if (!task) throw std::runtime_error("Invalid task");

  return task;
}
