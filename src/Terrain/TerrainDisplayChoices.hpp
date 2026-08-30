// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Shared Map Display → Terrain choice lists (Config panel and page
 * overrides).  Single source of truth for labels and ids.
 */

#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Terrain/TerrainSettings.hpp"

/** Terrain color ramp (ids match TerrainRendererSettings::ramp). */
static constexpr StaticEnumChoice terrain_ramp_choices[] = {
  { 0, N_("Low lands") },
  { 1, N_("Mountainous") },
  { 2, N_("Imhof 7") },
  { 3, N_("Imhof 4") },
  { 4, N_("Imhof 12") },
  { 5, N_("Imhof Atlas") },
  { 6, N_("ICAO") },
  { 9, N_("Vibrant") },
  { 7, N_("Grey") },
  { 8, N_("White") },
  { 10, N_("Sandstone") },
  { 11, N_("Pastel") },
  { 12, N_("Italian Avioportolano VFR Chart") },
  { 13, N_("German DFS VFR Chart") },
  { 14, N_("French SIA VFR Chart") },
  { 15, N_("High Contrast") },
  { 16, N_("High Contrast low lands") },
  { 17, N_("Very low lands") },
  nullptr
};

static constexpr StaticEnumChoice terrain_slope_shading_choices[] = {
  { SlopeShading::OFF, N_("Off") },
  { SlopeShading::FIXED, NC_("Setting", "Fixed (North-West)") },
  { SlopeShading::SUN, N_("Sun") },
  { SlopeShading::WIND, N_("Wind") },
  { SlopeShading::TOP_LEFT, NC_("Setting", "Fixed (Top Left)") },
  nullptr
};

static constexpr StaticEnumChoice terrain_contours_choices[] = {
  { Contours::OFF, N_("Off"), NC_("Setting", "No contour lines") },
  { Contours::MOUNTAINS, NC_("Setting", "Mountains"),
    N_("For steep mountain terrain, 256m minimum spacing") },
  { Contours::HIGHLANDS, NC_("Setting", "Highlands"),
    N_("Medium density, with 64m minimum spacing") },
  { Contours::LOWLANDS, NC_("Setting", "Lowlands"),
    N_("More line density for gentler slopes. 16m minimum spacing") },
  { Contours::SUPERFINE, NC_("Setting", "Superfine"),
    N_("Maximum density contour lines down to 8m spacing") },
  { Contours::FIXED_256, NC_("Setting", "Fixed 256m"),
    N_("Fixed 256m spacing, no zoom dependence") },
  { Contours::FIXED_128, NC_("Setting", "Fixed 128m"),
    N_("Fixed 128m spacing, no zoom dependence") },
  { Contours::FIXED_64, NC_("Setting", "Fixed 64m"),
    N_("Fixed 64m spacing, no zoom dependence") },
  nullptr
};

/** On/Off for terrain (and similar) boolean page overrides. */
static constexpr StaticEnumChoice terrain_enable_choices[] = {
  { 0, N_("Off") },
  { 1, N_("On") },
  nullptr
};

/** Convert terrain contrast/brightness byte (0..255) ↔ percent (0..100). */
[[nodiscard]] [[gnu::const]]
constexpr short
TerrainByteToPercent(short byte) noexcept
{
  return short((byte * 200 + 100) / 510);
}

[[nodiscard]] [[gnu::const]]
constexpr short
TerrainPercentToByte(short percent) noexcept
{
  return short((percent * 510 + 255) / 200);
}
