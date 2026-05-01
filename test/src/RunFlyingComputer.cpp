// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "system/Args.hpp"
#include "DebugReplay.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"
#include "Polar/Polar.hpp"
#include "Polar/PolarStore.hpp"
#include "Plane/Plane.hpp"
#include "ProductName.hpp"
#include "Version.hpp"
#include "system/StandardVersion.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

/** Canonical name for `--version` (do not derive from argv[0]). */
static constexpr const char canonical_name[] = "RunFlyingComputer";

/** Print usage and project URLs to stdout (for --help). */
static void
PrintStandardHelp() noexcept
{
  std::printf(
      "Usage: %s [OPTION]... <driver> <file>\n"
      "   or: %s [OPTION]... <file>\n"
      "\n"
      "Replay a flight recording and print take-off, release, and landing\n"
      "using the same FlyingComputer logic as XCSoar debug replay.\n"
      "\n"
      "Options:\n"
      "  --polar=NAME, --polar NAME   use PolarStore polar NAME "
      "(case-insensitive)\n"
      "  --list-polars               list polar names on standard output\n"
      "  -h, --help                  display this help and exit\n"
      "  --version                   output version information and exit\n"
      "\n"
      "If --polar is omitted, the built-in default glide polar is used.\n"
      "\n"
      "Report bugs to: <%s>\n"
      "%s home page: <%s>\n",
      canonical_name, canonical_name, PRODUCT_BUGS_URL, PRODUCT_NAME,
      PRODUCT_WEB_SITE_URL);
}

[[gnu::pure]]
static const PolarStore::Item *
FindPolarByName(const char *name) noexcept
{
  if (name == nullptr || *name == '\0')
    return nullptr;

  for (const auto &item : PolarStore::GetAll())
    if (StringIsEqualIgnoreCase(name, item.name))
      return &item;

  return nullptr;
}

static GlidePolar
GlidePolarFromStoreItem(const PolarStore::Item &item) noexcept
{
  const PolarInfo info = item.ToPolarInfo();
  const PolarCoefficients pc = info.CalculateCoefficients();
  if (!pc.IsValid())
    return GlidePolar::Invalid();

  GlidePolar gp(0);
  gp.SetCoefficients(pc, false);
  gp.SetReferenceMass(info.shape.reference_mass, false);
  gp.SetEmptyMass(item.empty_mass, false);
  const double crew =
    std::max(0., item.reference_mass - double(item.empty_mass));
  gp.SetCrewMass(crew, false);
  gp.SetMaxBallast(info.max_ballast, false);
  gp.SetWingArea(info.wing_area);
  gp.SetVMax(DEFAULT_MAX_SPEED, false);
  gp.Update();

  if (info.v_no > 0 && gp.IsValid())
    gp.SetVMax(std::clamp(info.v_no, gp.GetVMin() + 10, DEFAULT_MAX_SPEED));

  return gp;
}

static void
ListPolars() noexcept
{
  for (const auto &item : PolarStore::GetAll())
    puts(item.name);
}

static void
ApplyPolarOption(bool &have_override, GlidePolar &polar_override,
                 GlidePolar new_polar, const char *context_for_error) noexcept
{
  if (have_override) {
    fprintf(stderr, "Only one --polar= option allowed.\n");
    exit(EXIT_FAILURE);
  }

  if (!new_polar.IsValid()) {
    fprintf(stderr, "%s\n", context_for_error);
    exit(EXIT_FAILURE);
  }

  polar_override = new_polar;
  have_override = true;
}

/** Consume optional flags at the start of the command line. */
static void
ParsePolarCliOptions(Args &args, bool &have_override,
                     GlidePolar &polar_override, bool &list_polars) noexcept
{
  have_override = false;
  list_polars = false;

  while (!args.IsEmpty()) {
    const char *peek = args.PeekNext();

    if (StringIsEqual(peek, "--help") || StringIsEqual(peek, "-h")) {
      args.Skip();
      PrintStandardHelp();
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(peek, "--version")) {
      args.Skip();
      PrintStandardVersion(canonical_name, XCSoar_Version);
      std::exit(EXIT_SUCCESS);
    }

    if (StringIsEqual(peek, "--list-polars")) {
      args.Skip();
      list_polars = true;
      continue;
    }

    if (const char *polar_name =
          StringAfterPrefixIgnoreCase(peek, "--polar=");
        polar_name != nullptr) {
      args.Skip();
      if (*polar_name == '\0')
        args.UsageError();

      const PolarStore::Item *item = FindPolarByName(polar_name);
      if (item == nullptr) {
        fprintf(stderr, "Unknown polar '%s'. Try --list-polars.\n",
                polar_name);
        exit(EXIT_FAILURE);
      }

      ApplyPolarOption(have_override, polar_override,
                       GlidePolarFromStoreItem(*item),
                       "Invalid polar coefficients.");
      continue;
    }

    if (StringIsEqualIgnoreCase(peek, "--polar")) {
      args.Skip();
      const char *polar_name = args.ExpectNext();
      if (*polar_name == '\0')
        args.UsageError();

      const PolarStore::Item *item = FindPolarByName(polar_name);
      if (item == nullptr) {
        fprintf(stderr, "Unknown polar '%s'. Try --list-polars.\n",
                polar_name);
        exit(EXIT_FAILURE);
      }

      ApplyPolarOption(have_override, polar_override,
                       GlidePolarFromStoreItem(*item),
                       "Invalid polar coefficients.");
      continue;
    }

    break;
  }
}

static void
LogEvent(const char *event, TimeStamp time, const GeoPoint &location) noexcept
{
  char time_buffer[32];
  FormatTime(time_buffer, time);

  printf("%s %s %s\n", time_buffer,
         FormatGeoPoint(location, CoordinateFormat::DDMMSS).c_str(),
         event);
}

int
main(int argc, char **argv)
{
  Args args(argc, argv,
            "[OPTION]... <driver> <file>\n"
            "       RunFlyingComputer [OPTION]... <file>");

  GlidePolar polar_override;
  bool have_polar_override = false;
  bool list_polars = false;
  ParsePolarCliOptions(args, have_polar_override, polar_override, list_polars);

  if (list_polars) {
    ListPolars();
    args.ExpectEnd();
    return EXIT_SUCCESS;
  }

  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == nullptr)
    return EXIT_FAILURE;

  if (have_polar_override)
    replay->SetGlidePolar(std::move(polar_override));

  args.ExpectEnd();

  bool last_flying = false, last_released = false;

  while (replay->Next()) {
    const FlyingState &flight = replay->Calculated().flight;

    if (flight.flying && !last_flying)
      LogEvent("take-off", flight.takeoff_time, flight.takeoff_location);
    else if (!flight.flying && last_flying)
      LogEvent("landing", flight.landing_time, flight.landing_location);
    else if (flight.release_time.IsDefined() && !last_released)
      LogEvent("release", flight.release_time, flight.release_location);

    last_flying = flight.flying;
    last_released = flight.release_time.IsDefined();
  }

  const FlyingState &flight = replay->Calculated().flight;
  if (flight.far_distance >= 0)
    printf("far %u km at %s\n", unsigned(flight.far_distance / 1000),
           FormatGeoPoint(flight.far_location,
                          CoordinateFormat::DDMMSS).c_str());

  delete replay;
  return EXIT_SUCCESS;
}
