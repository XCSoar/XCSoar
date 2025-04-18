// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

struct AirspaceRendererSettings;
struct AirspaceClassRendererSettings;
struct AirspaceComputerSettings;
class RGB8Color;
class ProfileMap;

namespace Profile
{
  void Load(const ProfileMap &map, AirspaceRendererSettings &renderer);
  void Load(const ProfileMap &map,
            unsigned i, AirspaceClassRendererSettings &settings);
  void Load(const ProfileMap &map, AirspaceComputerSettings &computer);

  /**
   * Saves the airspace mode setting to the profile
   * @param i Airspace class index
   */
  void SetAirspaceMode(ProfileMap &map, unsigned i, bool display, bool warning);
  void SetAirspaceBorderWidth(ProfileMap &map,
                              unsigned i, unsigned border_width);
  void SetAirspaceBorderColor(ProfileMap &map,
                              unsigned i, const RGB8Color &color);
  void SetAirspaceFillColor(ProfileMap &map,
                            unsigned i, const RGB8Color &color);
  void SetAirspaceFillMode(ProfileMap &map, unsigned i, uint8_t mode);
  void SetAirspaceBrush(ProfileMap &map, unsigned i, int c);
};
