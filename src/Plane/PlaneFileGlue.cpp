// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlaneFileGlue.hpp"
#include "Plane.hpp"
#include "Polar/Parser.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "io/KeyValueFileReader.hpp"
#include "io/KeyValueFileWriter.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/FileLineReader.hpp"
#include "system/FileUtil.hpp"
#include "system/Path.hpp"
#include "LocalPath.hpp"
#include "util/NumberParser.hpp"
#include "LogFile.hpp"

#include <fmt/format.h>

static bool
ReadPolar(const char *string, Plane &plane) noexcept
{
  return ParsePolarShape(plane.polar_shape, string);
}

static bool
ReadDouble(const char *string, double &out) noexcept
{
  char *endptr;
  double tmp = ParseDouble(string, &endptr);
  if (endptr == string)
    return false;

  out = tmp;
  return true;
}

static bool
ReadUnsigned(const char *string, unsigned &out) noexcept
{
  char *endptr;
  unsigned tmp = ParseUnsigned(string, &endptr, 0);
  if (endptr == string)
    return false;

  out = tmp;
  return true;
}

bool
PlaneGlue::Read(Plane &plane, KeyValueFileReader &reader)
{
  bool has_registration = false;
  bool has_competition_id = false;
  bool has_type = false;
  bool has_weglide_type = false;
  bool has_polar_name = false;
  bool has_polar = false;
  bool has_reference_mass = false;
  bool has_dry_mass = false;
  bool has_empty_mass = false;
  bool has_handicap = false;
  bool has_max_ballast = false;
  bool has_dump_time = false;
  bool has_max_speed = false;
  bool has_wing_area = false;

  KeyValuePair pair;
  while (reader.Read(pair)) {
    if (!has_registration && StringIsEqual(pair.key, "Registration")) {
      plane.registration.SetUTF8(pair.value);
      has_registration = true;
    } else if (!has_competition_id && StringIsEqual(pair.key, "CompetitionID")) {
      plane.competition_id.SetUTF8(pair.value);
      has_competition_id = true;
    } else if (!has_type && StringIsEqual(pair.key, "Type")) {
      plane.type.SetUTF8(pair.value);
      has_type = true;
    } else if (!has_weglide_type &&
      StringIsEqual(pair.key, "WeGlideAircraftType")) {
      has_weglide_type = ReadUnsigned(pair.value, plane.weglide_glider_type);
    } else if (!has_handicap && StringIsEqual(pair.key, "Handicap")) {
      has_handicap = ReadUnsigned(pair.value, plane.handicap);
    } else if (!has_polar_name && StringIsEqual(pair.key, "PolarName")) {
      plane.polar_name.SetUTF8(pair.value);
      has_polar_name = true;
    } else if (!has_polar && StringIsEqual(pair.key, "PolarInformation")) {
      has_polar = ReadPolar(pair.value, plane);
    } else if (!has_reference_mass && StringIsEqual(pair.key, "PolarReferenceMass")) {
      has_reference_mass = ReadDouble(pair.value, plane.polar_shape.reference_mass);
    } else if (!has_dry_mass && StringIsEqual(pair.key, "PolarDryMass")) {
      has_dry_mass = ReadDouble(pair.value, plane.dry_mass_obsolete);
    } else if (!has_empty_mass && StringIsEqual(pair.key, "PlaneEmptyMass")) {
      has_empty_mass = ReadDouble(pair.value, plane.empty_mass);
    } else if (!has_max_ballast && StringIsEqual(pair.key, "MaxBallast")) {
      has_max_ballast = ReadDouble(pair.value, plane.max_ballast);
    } else if (!has_dump_time && StringIsEqual(pair.key, "DumpTime")) {
      has_dump_time = ReadUnsigned(pair.value, plane.dump_time);
    } else if (!has_max_speed && StringIsEqual(pair.key, "MaxSpeed")) {
      has_max_speed = ReadDouble(pair.value, plane.max_speed);
    } else if (!has_wing_area && StringIsEqual(pair.key, "WingArea")) {
      has_wing_area = ReadDouble(pair.value, plane.wing_area);
    }
  }

  if (!has_polar || !has_reference_mass)
    return false;

  if (!has_registration)
    plane.registration.clear();
  if (!has_competition_id)
    plane.competition_id.clear();
  if (!has_type)
    plane.type.clear();
  if (!has_weglide_type)
    plane.weglide_glider_type = 0;
  if (!has_polar_name)
    plane.polar_name.clear();
  if (!has_empty_mass && has_dry_mass) {
    plane.empty_mass = plane.dry_mass_obsolete - 90.;
    has_empty_mass = true;
  }
  if (!has_empty_mass)
    plane.empty_mass = plane.polar_shape.reference_mass;
  if (!has_handicap || plane.handicap == 0)
    plane.handicap = 100;
  if (!has_max_ballast)
    plane.max_ballast = 0;
  if (!has_dump_time)
    plane.dump_time = 0;
  if (!has_max_speed)
    plane.max_speed = 55.555;
  if (!has_wing_area)
    plane.wing_area = 0;

  return true;
}

bool
PlaneGlue::ReadFile(Plane &plane, Path path) noexcept
try {
  FileLineReaderA reader(path);
  KeyValueFileReader kvreader(reader);
  if (Read(plane, kvreader)) {
    plane.plane_profile_active = true;
    return true;
  }
  plane.plane_profile_active = false;
  return false;
} catch (...) {
  LogError(std::current_exception());
  plane.plane_profile_active = false;
  return false;
}

void
PlaneGlue::Write(const Plane &plane, KeyValueFileWriter &writer)
{
  StaticString<255> tmp;

  writer.Write("Registration", plane.registration);
  writer.Write("CompetitionID", plane.competition_id);
  writer.Write("Type", plane.type);

  tmp.Format("%u", plane.handicap);
  writer.Write("Handicap", tmp);

  writer.Write("PolarName", plane.polar_name);

  FormatPolarShape(plane.polar_shape, tmp.buffer(), tmp.capacity());
  writer.Write("PolarInformation", tmp);

  tmp.Format("%f", (double)plane.polar_shape.reference_mass);
  writer.Write("PolarReferenceMass", tmp);
  tmp.Format("%f", (double)plane.dry_mass_obsolete);  // dry mass split into empty and crew masses
                                                      // keep entry for temporary backward compatibility
  writer.Write("PolarDryMass", tmp);
  tmp.Format("%f", (double)plane.empty_mass);
  writer.Write("PlaneEmptyMass", tmp);
  tmp.Format("%f", (double)plane.max_ballast);
  writer.Write("MaxBallast", tmp);
  tmp.Format("%f", (double)plane.dump_time);
  writer.Write("DumpTime", tmp);
  tmp.Format("%f", (double)plane.max_speed);
  writer.Write("MaxSpeed", tmp);
  tmp.Format("%f", (double)plane.wing_area);
  writer.Write("WingArea", tmp);
  tmp.Format("%u", (unsigned)plane.weglide_glider_type);
  writer.Write("WeGlideAircraftType", tmp);
}

void
PlaneGlue::WriteFile(const Plane &plane, Path path)
{
  FileOutputStream file(path);
  BufferedOutputStream buffered(file);
  KeyValueFileWriter kvwriter(buffered);
  Write(plane, kvwriter);
  buffered.Flush();
  file.Commit();
}

AllocatedPath
PlaneGlue::FindByRegistration(const char *registration)
{
  if (registration == nullptr || *registration == '\0')
    return nullptr;

  struct PlaneMatch {
    const char *target_reg;
    AllocatedPath found_path{nullptr};
  } match{registration};

  class MatchVisitor : public File::Visitor {
    PlaneMatch &match;
  public:
    explicit MatchVisitor(PlaneMatch &m) : match(m) {}
    void Visit(Path path, [[maybe_unused]] Path filename) override {
      if (match.found_path != nullptr)
        return;
      Plane p{};
      if (PlaneGlue::ReadFile(p, path) &&
          p.registration.equals(match.target_reg))
        match.found_path = AllocatedPath{path};
    }
  } visitor{match};

  VisitDataFiles("*.xcp", visitor);
  return std::move(match.found_path);
}

/**
 * Strip characters that are unsafe for use in filenames.
 * Only alphanumeric, dash and underscore are kept.
 */
static std::string
SanitizeFilename(const char *s) noexcept
{
  std::string result;
  for (; *s != '\0'; ++s) {
    char c = *s;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_')
      result += c;
  }
  return result;
}

AllocatedPath
PlaneGlue::CreateFromPolar(const char *registration,
                           const char *competition_id,
                           const char *glider_type,
                           const GlidePolar &gp)
{
  if (registration == nullptr || *registration == '\0')
    return nullptr;

  if (!gp.IsValid())
    return nullptr;

  Plane plane{};
  plane.registration.SetUTF8(registration);
  if (competition_id != nullptr && *competition_id != '\0')
    plane.competition_id.SetUTF8(competition_id);
  if (glider_type != nullptr && *glider_type != '\0')
    plane.type.SetUTF8(glider_type);
  plane.polar_name.clear();

  plane.polar_shape.reference_mass = gp.GetReferenceMass();
  plane.empty_mass = gp.GetEmptyMass();
  plane.wing_area = gp.GetWingArea();
  plane.max_speed = DEFAULT_MAX_SPEED;
  plane.max_ballast = gp.GetMaxBallast();
  plane.handicap = 100;

  const auto &coeffs = gp.GetCoefficients();
  if (coeffs.IsValid()) {
    constexpr double speeds[] = {90.0 / 3.6, 130.0 / 3.6, 180.0 / 3.6};
    for (unsigned i = 0; i < 3; ++i) {
      const double v = speeds[i];
      plane.polar_shape.points[i].v = v;
      plane.polar_shape.points[i].w =
        coeffs.a * v * v + coeffs.b * v + coeffs.c;
    }
  }

  const auto safe_name = SanitizeFilename(registration);
  if (safe_name.empty())
    return nullptr;

  const auto filename = fmt::format("{}.xcp", safe_name);
  auto path = LocalPath(filename.c_str());

  WriteFile(plane, path);
  return path;
}
