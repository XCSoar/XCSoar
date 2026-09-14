// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DefaultFonts.hpp"
#include "GlobalFonts.hpp"
#include "FontSettings.hpp"
#include "Screen/Layout.hpp"

namespace Fonts {

static void
InitialiseLogFonts(FontSettings &settings) noexcept
{
  settings.map = FontDescription(Layout::FontScale(12));
  settings.map_bold = FontDescription(Layout::FontScale(12), true);
}

FontSettings
GetDefaults() noexcept
{
  FontSettings settings;
  InitialiseLogFonts(settings);
  return settings;
}

void
Initialize()
{
  const auto default_settings = GetDefaults();

  Load(default_settings);
}

} // namespace Fonts
