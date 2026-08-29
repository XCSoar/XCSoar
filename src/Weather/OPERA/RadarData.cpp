// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Radar.hpp"
#include "Geo/GeoPoint.hpp"
#include "time/BrokenDateTime.hpp"
#include "Math/Constants.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <functional>

namespace {

constexpr const char *BUCKET_URL =
  "https://s3.waw3-1.cloudferro.com/openradar-24h";

/* the Lambert azimuthal equal-area grid the composite is published
   on, read from the GeoTIFF's own keys */
constexpr double EARTH_RADIUS = 6378140;
constexpr double CENTRE_LATITUDE = 55;
constexpr double CENTRE_LONGITUDE = 10;
constexpr double FALSE_EASTING = 1950000;
constexpr double FALSE_NORTHING = -2100000;

/**
 * The lower reflectivity bound of each colour class.
 *
 * The last entry is the "everything above" class; a value below the
 * first one is not drawn at all, which is what the reference product
 * does with the weakest echoes.
 */
/**
 * The lower dBZ bound of each colour class, obtained by matching the
 * quantiles of the composite against a DWD European radar image.
 *
 * Classes 1 and 9 were absent from that image, so the match left them
 * with an empty range; their bounds are interpolated evenly in dBZ
 * (that is, geometrically in rain rate) between the neighbours it did
 * determine.  The array must stay strictly increasing, or
 * ClassifyReflectivity() would step over a class and one colour would
 * never be drawn.
 */
constexpr std::array<double, OPERA::N_CLASSES> CLASS_MINIMUM{
  18.77, 21.16, 23.56, 25.95, 30.43, 34.62, 38.68, 42.82,
  47.81, 50.77, 53.73, 56.69, 60.70, 62.96, 66.14,
};

static_assert(std::ranges::adjacent_find(CLASS_MINIMUM,
                                         std::greater_equal<>{}) ==
              CLASS_MINIMUM.end(),
              "the class bounds must be strictly increasing");

constexpr std::array<uint32_t, OPERA::N_CLASSES> CLASS_COLOUR{
  0x33ffff, 0x1acc9a, 0x019934, 0x4db31b, 0x99cc01,
  0xcce601, 0xffff01, 0xffc401, 0xff8901, 0xff4501,
  0xfe0000, 0xe5004c, 0xcc0098, 0x6600cb, 0x0000fe,
};

} // anonymous namespace

BrokenDateTime
OPERA::CompositeTime(const BrokenDateTime &utc)
{
  if (!utc.IsPlausible())
    /* without a clock we cannot tell which composite is the current
       one, and guessing would show yesterday's weather */
    return BrokenDateTime::Invalid();

  auto t = utc - std::chrono::minutes{LATENCY_MINUTES};
  t.minute = (t.minute / CADENCE_MINUTES) * CADENCE_MINUTES;
  t.second = 0;
  return t;
}

std::string
OPERA::MakeCompositeURL(const BrokenDateTime &utc)
{
  const auto t = CompositeTime(utc);
  if (!t.IsPlausible())
    return {};

  return fmt::format("{}/{:04}/{:02}/{:02}/OPERA/COMP/"
                     "OPERA@{:04}{:02}{:02}T{:02}{:02}@0@DBZH.tiff",
                     BUCKET_URL, t.year, t.month, t.day,
                     t.year, t.month, t.day, t.hour, t.minute);
}

DoublePoint2D
OPERA::Project(const GeoPoint &p) noexcept
{
  const double lat = p.latitude.Radians();
  const double lon = p.longitude.Radians();
  const double lat0 = CENTRE_LATITUDE * M_PI / 180;
  const double delta = lon - CENTRE_LONGITUDE * M_PI / 180;

  const double denominator = 1 + std::sin(lat0) * std::sin(lat) +
    std::cos(lat0) * std::cos(lat) * std::cos(delta);
  if (denominator <= 0)
    /* the antipode of the projection centre has no image */
    return {0, 0};

  const double k = EARTH_RADIUS * std::sqrt(2 / denominator);

  return {
    FALSE_EASTING + k * std::cos(lat) * std::sin(delta),
    FALSE_NORTHING + k * (std::cos(lat0) * std::sin(lat) -
                          std::sin(lat0) * std::cos(lat) * std::cos(delta)),
  };
}

bool
OPERA::IsNotANumber(double value) noexcept
{
  uint64_t bits;
  static_assert(sizeof(bits) == sizeof(value));
  std::memcpy(&bits, &value, sizeof(bits));

  return ((bits >> 52) & 0x7ff) == 0x7ff && (bits & 0xfffffffffffffULL) != 0;
}

int
OPERA::ClassifyReflectivity(double dbz) noexcept
{
  if (IsNotANumber(dbz) || dbz < CLASS_MINIMUM.front())
    return -1;

  unsigned i = 0;
  while (i + 1 < N_CLASSES && dbz >= CLASS_MINIMUM[i + 1])
    ++i;

  return int(i);
}

uint32_t
OPERA::GetClassColour(unsigned i) noexcept
{
  return CLASS_COLOUR[i < N_CLASSES ? i : N_CLASSES - 1];
}
