// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlobalFonts.hpp"
#include "FontSettings.hpp"
#include "ui/canvas/Font.hpp"

namespace Fonts {

/// text names on the map
Font map;
/// menu buttons, waypoint selection, messages, etc.
Font map_bold;

void
Load(const FontSettings &settings)
{
  map.Load(settings.map);
  map_bold.Load(settings.map_bold);
}

void
Deinitialize() noexcept
{
  map.Destroy();
  map_bold.Destroy();
}

} // namespace Fonts
