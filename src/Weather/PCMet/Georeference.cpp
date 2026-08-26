// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Georeference.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>
#include <cmath>

namespace {

/* The parameters of the DWD polar stereographic grid, as published in
   "RADOLAN und RADVOR: Beschreibung des Kompositformats" version
   2.5.8, chapter 1.3 "Georeferenzierung": the projection plane cuts
   the sphere at 60°N, is aligned with the 10°E meridian, and the
   earth is a sphere of radius 6370.04km with zero eccentricity. */
constexpr double EARTH_RADIUS = 6370.04;
constexpr double STANDARD_PARALLEL = 60;
constexpr double CENTRAL_MERIDIAN = 10;

struct AreaGeoreference {
  const char *area;
  PCMet::ImageGeoreference georeference;
};

/**
 * The map sections of the "Satellitenbilder" products.  All three
 * channels (vis_hrv, ir_rgb, ir_108) of one area share the same
 * section, therefore the suffix of the area name selects the entry.
 *
 * These were measured on downloaded images.  The DWD draws a marker
 * for each major airport, and the two overview images additionally
 * carry a 5° graticule; fitting the projection to either gives the
 * same result to within 0.06% in resolution and half a pixel in
 * position.  The residuals are 0.4 to 0.6 pixel, and the three German
 * sections come out at the same resolution independently, which is a
 * good sign that the numbers are sound.
 */
constexpr AreaGeoreference sat_georeferences[] = {
  /* "Europa" */
  { "eu", { {1000, 750}, 6.4813, { 563.728, -287.134 } } },

  /* "Mitteleuropa" */
  { "ce", { {1000, 750}, 2.5819, { 566.107, -1249.418 } } },

  /* "Deutschland" and its northern and southern half */
  { "mdl", { {1000, 750}, 1.1987, { 488.056, -3148.038 } } },
  { "ndl", { {1000, 750}, 1.1984, { 510.923, -2816.887 } } },
  { "sdl", { {1000, 750}, 1.1993, { 460.644, -3586.498 } } },
};

/**
 * Extract the map section suffix of a satellite area name, e.g.
 * "mdl" from "vis_hrv_mdl".
 */
[[gnu::pure]]
const char *
GetSatAreaSuffix(const char *area) noexcept
{
  const char *underscore = StringFindLast(area, '_');
  return underscore != nullptr ? underscore + 1 : area;
}

} // anonymous namespace

DoublePoint2D
PCMet::ImageGeoreference::ToPixel(const GeoPoint &p) const noexcept
{
  /* distance from the pole in the projection plane; the scale factor
     makes the projection true at STANDARD_PARALLEL */
  const double scale = (1 + Angle::Degrees(STANDARD_PARALLEL).sin())
    / (1 + p.latitude.sin());
  const double radius = EARTH_RADIUS * scale * p.latitude.cos();

  const Angle delta = p.longitude - Angle::Degrees(CENTRAL_MERIDIAN);
  const double x = radius * delta.sin();
  const double y = -radius * delta.cos();

  /* the projection y axis points north, the image y axis points down */
  return {
    pole.x + x / resolution,
    pole.y - y / resolution,
  };
}

Angle
PCMet::ImageGeoreference::GetUpBearing(const GeoPoint &p) const noexcept
{
  return p.longitude - Angle::Degrees(CENTRAL_MERIDIAN);
}

bool
PCMet::ImageGeoreference::IsInside(const DoublePoint2D pixel) const noexcept
{
  return pixel.x >= 0 && pixel.x < nominal_size.width &&
    pixel.y >= 0 && pixel.y < nominal_size.height;
}

const PCMet::ImageGeoreference *
PCMet::FindImageGeoreference(const char *type_uri,
                             const char *area_name) noexcept
{
  if (type_uri == nullptr || area_name == nullptr)
    /* the image tables are terminated by a null entry, so a caller
       walking one past the end would land here */
    return nullptr;

  if (!StringIsEqual(type_uri, "sat/index.htm"))
    /* only the satellite images are georeferenced so far */
    return nullptr;

  const char *suffix = GetSatAreaSuffix(area_name);

  const auto i = std::find_if(std::begin(sat_georeferences),
                              std::end(sat_georeferences),
                              [suffix](const AreaGeoreference &a){
                                return StringIsEqual(a.area, suffix);
                              });

  return i != std::end(sat_georeferences) ? &i->georeference : nullptr;
}
