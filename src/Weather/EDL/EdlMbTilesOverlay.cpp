// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlMbTilesOverlay.hpp"

#include "Language/Language.hpp"
#include "Weather/EDL/TileValue.hpp"
#include "lib/fmt/ToBuffer.hxx"
#include "util/StringAPI.hxx"

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

bool
EdlMbTilesOverlay::FormatPointInfo(GeoPoint p, char *buffer,
                                   std::size_t size) const noexcept
{
  if (buffer == nullptr || size == 0)
    return false;

  double value_mps = 0;
  if (!SampleAscendancyAt(p, value_mps)) {
    CopyString(buffer, size, _("[no data]"));
    return true;
  }

  const auto line = FmtBuffer<32>("{:+.1f} m/s", value_mps);
  CopyString(buffer, size, line.c_str());
  return true;
}
