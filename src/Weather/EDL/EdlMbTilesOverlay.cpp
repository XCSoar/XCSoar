// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlMbTilesOverlay.hpp"

#include "Weather/EDL/TileValue.hpp"

EdlMbTilesOverlay::EdlMbTilesOverlay(Path path, std::string label)
  :MbTilesOverlay(std::move(path), std::move(label))
{
}

bool
EdlMbTilesOverlay::SampleAscendancyAt(GeoPoint p,
                                      double &value_mps) const noexcept
{
  Rgba8 rgba;
  if (!SampleRgbaAtGeo(p, rgba))
    return false;

  return EDL::DecodeAscendancyPixel(rgba.r, rgba.g, rgba.b, rgba.a,
                                    value_mps);
}
