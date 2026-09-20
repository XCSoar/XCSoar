// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Georeference.hpp"
#include "Geo/GeoPoint.hpp"
#include "Math/Angle.hpp"
#include "util/StringAPI.hxx"

#include <algorithm>
#include <cmath>
#include <span>

namespace {

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
  { "eu", { {1000, 750}, 750, 6.4813, { 563.728, -287.134 } } },

  /* "Mitteleuropa" */
  { "ce", { {1000, 750}, 750, 2.5819, { 566.107, -1249.418 } } },

  /* "Deutschland" and its northern and southern half */
  { "mdl", { {1000, 750}, 750, 1.1987, { 488.056, -3148.038 } } },
  { "ndl", { {1000, 750}, 750, 1.1984, { 510.923, -2816.887 } } },
  { "sdl", { {1000, 750}, 750, 1.1993, { 460.644, -3586.498 } } },
};

/**
 * The RADAR composites.  Unlike the satellite images these carry a
 * legend strip below the map, so #map_height stops short of the image
 * height.
 *
 * Measured the same way as the satellite sections: the central
 * meridian is the only one drawn vertically, which fixes pole.x, and
 * the latitude circles are arcs about the pole, which fixes pole.y
 * and the resolution once their latitudes are read off the margin.
 */
constexpr AreaGeoreference rad_georeferences[] = {
  /* "Deutschland", the only product that is not 1000 pixels wide */
  { "rx", { {600, 750}, 698, 1.5493, { 288.000, -2367.354 } } },

  /* "Deutschland Nord", "Mitte" and "Süd" */
  { "rxn", { {1000, 750}, 698, 0.8864, { 479.000, -4123.050 } } },
  { "rxm", { {1000, 750}, 698, 0.8605, { 477.000, -4597.030 } } },
  { "rxs", { {1000, 750}, 698, 0.8581, { 492.000, -4829.360 } } },

  /* "Europa"; the graticule is drawn every ten degrees here, so this
     one rests on fewer reference points than the others */
  { "eu", { {1000, 750}, 697, 3.2799, { 581.000, -968.000 } } },

  /* "Alpen" */
  { "fa", { {1000, 750}, 697, 1.0745, { 457.000, -4099.720 } } },
};

/** The combined satellite, radar and lightning image. */
constexpr AreaGeoreference satradblitz_georeferences[] = {
  /* "Europa" */
  { "eh", { {1000, 750}, 727, 2.3699, { 625.000, -1446.530 } } },
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
    pixel.y >= 0 && pixel.y < map_height;
}

const PCMet::ImageGeoreference *
PCMet::FindImageGeoreference(const char *type_uri,
                             const char *area_name) noexcept
{
  if (type_uri == nullptr || area_name == nullptr)
    /* the image tables are terminated by a null entry, so a caller
       walking one past the end would land here */
    return nullptr;

  std::span<const AreaGeoreference> table;
  const char *key = area_name;

  if (StringIsEqual(type_uri, "sat/index.htm")) {
    table = sat_georeferences;
    /* the satellite areas name the channel as well, e.g. "ir_108_mdl",
       but all channels of one area show the same map section */
    key = GetSatAreaSuffix(area_name);
  } else if (StringIsEqual(type_uri, "rad/index.htm")) {
    table = rad_georeferences;
  } else if (StringIsEqual(type_uri, "satradblitz/index.htm")) {
    table = satradblitz_georeferences;
  } else
    /* the local radar images and the lightning map are not
       georeferenced */
    return nullptr;

  const auto i = std::find_if(table.begin(), table.end(),
                              [key](const AreaGeoreference &a){
                                return StringIsEqual(a.area, key);
                              });

  return i != table.end() ? &i->georeference : nullptr;
}
