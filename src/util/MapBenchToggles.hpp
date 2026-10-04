// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdlib>
#include <cstring>

/**
 * A/B switches for RunMapRendererStress.  Empty env = current WIP.
 *
 * XCSOAR_BENCH_TOPO: comma-separated
 *   head        — committed thinning (layer scale_threshold, 1 px bbox)
 *   nocollapse  — keep dual carriageways
 *   noline      — do not skip short OSM sticks
 *   nofill      — no extra fill/landcover spacing
 *
 * XCSOAR_BENCH_AS:
 *   geo   — geographic even-odd + ToGLM (default)
 *   clip  — even-odd after ClipPolygon each frame
 *   cache — ear-clip once, geographic triangle cache
 *   ear   — ear-clip every frame (pre-cache)
 */
inline const char *
MapBenchTopoEnv() noexcept
{
  const char *e = std::getenv("XCSOAR_BENCH_TOPO");
  return e != nullptr ? e : "";
}

inline bool
MapBenchTopoHas(const char *token) noexcept
{
  return std::strstr(MapBenchTopoEnv(), token) != nullptr;
}

inline bool
MapBenchTopoHead() noexcept
{
  return MapBenchTopoHas("head");
}

enum class MapBenchAirspace {
  Geo,
  Clip,
  Cache,
  Ear,
};

inline MapBenchAirspace
GetMapBenchAirspace() noexcept
{
  const char *e = std::getenv("XCSOAR_BENCH_AS");
  if (e == nullptr || *e == '\0' || std::strcmp(e, "geo") == 0)
    return MapBenchAirspace::Geo;
  if (std::strcmp(e, "clip") == 0)
    return MapBenchAirspace::Clip;
  if (std::strcmp(e, "cache") == 0)
    return MapBenchAirspace::Cache;
  if (std::strcmp(e, "ear") == 0)
    return MapBenchAirspace::Ear;
  return MapBenchAirspace::Geo;
}
