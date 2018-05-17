/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "AirspaceConfig.hpp"
#include "Map.hpp"
#include "ProfileKeys.hpp"
#include "Screen/Features.hpp"
#include "Look/AirspaceLook.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Util/Macros.hpp"

#include <string.h>
#include <stdio.h>

static const char *
MakeAirspaceSettingName(char *buffer, const char *prefix, unsigned n)
{
  strcpy(buffer, prefix);
  sprintf(buffer + strlen(buffer), "%u", n);

  return buffer;
}

/**
 * This function and the "ColourXX" profile keys are deprecated and
 * are only used as a fallback for old profiles.
 *
 * @see Load(unsigned, AirspaceClassRendererSettings &)
 */
static bool
GetAirspaceColor(const ProfileMap &map, unsigned i, RGB8Color &color)
{
  char name[64];
  MakeAirspaceSettingName(name, "Colour", i);

  // Try to load the hex color directly
  if (map.GetColor(name, color))
    return true;

  // Try to load an indexed preset color (legacy, < 6.3)
  unsigned index;
  if (!map.Get(name, index))
    return false;

  // Adjust index if the user has configured a preset color out of range
  if (index >= ARRAY_SIZE(AirspaceLook::preset_colors))
    index = 0;

  // Assign configured preset color
  color = AirspaceLook::preset_colors[index];
  return true;
}

void
Profile::Load(const ProfileMap &map, AirspaceRendererSettings &settings)
{
  map.GetEnum(ProfileKeys::AirspaceLabelSelection, settings.label_selection);
  map.Get(ProfileKeys::AirspaceBlackOutline, settings.black_outline);
  map.GetEnum(ProfileKeys::AltMode, settings.altitude_mode);
  map.Get(ProfileKeys::ClipAlt, settings.clip_altitude);

#if defined(HAVE_HATCHED_BRUSH) && defined(HAVE_ALPHA_BLEND)
  map.Get(ProfileKeys::AirspaceTransparency, settings.transparency);
#endif

  map.GetEnum(ProfileKeys::AirspaceFillMode, settings.fill_mode);

  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++)
    Load(map, i, settings.classes[i]);
}

void
Profile::Load(const ProfileMap &map,
              unsigned i, AirspaceClassRendererSettings &settings)
{
  char name[64];

  MakeAirspaceSettingName(name, "AirspaceDisplay", i);
  if (!map.Get(name, settings.display)) {
    // Load setting from legacy key-value pair
    MakeAirspaceSettingName(name, "AirspaceMode", i);

    unsigned value;
    if (map.Get(name, value))
      settings.display = (value & 0x1) != 0;
  }

#ifdef HAVE_HATCHED_BRUSH
  MakeAirspaceSettingName(name, "Brush", i);
  map.Get(name, settings.brush);
  if (settings.brush >= ARRAY_SIZE(AirspaceLook::brushes))
    settings.brush = 0;
#endif

  MakeAirspaceSettingName(name, "AirspaceBorderColor", i);
  if (!map.GetColor(name, settings.border_color))
    GetAirspaceColor(map, i, settings.border_color);

  MakeAirspaceSettingName(name, "AirspaceFillColor", i);
  if (!map.GetColor(name, settings.fill_color))
    GetAirspaceColor(map, i, settings.fill_color);

  MakeAirspaceSettingName(name, "AirspaceBorderWidth", i);
  map.Get(name, settings.border_width);

  MakeAirspaceSettingName(name, "AirspaceFillMode", i);
  map.GetEnum(name, settings.fill_mode);
}

void
Profile::Load(const ProfileMap &map, AirspaceComputerSettings &settings)
{
  map.Get(ProfileKeys::AirspaceWarning, settings.enable_warnings);
  map.Get(ProfileKeys::AltMargin, settings.warnings.altitude_warning_margin);
  map.Get(ProfileKeys::WarningTime, settings.warnings.warning_time);
  map.Get(ProfileKeys::AcknowledgementTime,
          settings.warnings.acknowledgement_time);
  map.Get(ProfileKeys::RepetitiveSound, settings.warnings.repetitive_sound);

  char name[64];
  unsigned value;
  for (unsigned i = 0; i < AIRSPACECLASSCOUNT; i++) {
    MakeAirspaceSettingName(name, "AirspaceWarning", i);
    if (!map.Get(name, settings.warnings.class_warnings[i])) {
      // Load setting from legacy key-value pair
      MakeAirspaceSettingName(name, "AirspaceMode", i);
      if (map.Get(name, value))
        settings.warnings.class_warnings[i] = (value & 0x2) != 0;
    }
  }
}

void
Profile::SetAirspaceMode(ProfileMap &map,
                         unsigned i, bool display, bool warning)
{
  char name[64];

  MakeAirspaceSettingName(name, "AirspaceDisplay", i);
  map.Set(name, display);

  MakeAirspaceSettingName(name, "AirspaceWarning", i);
  map.Set(name, warning);
}

void
Profile::SetAirspaceBorderWidth(ProfileMap &map,
                                unsigned i, unsigned border_width)
{
  char name[64];
  MakeAirspaceSettingName(name, "AirspaceBorderWidth", i);
  map.Set(name, border_width);
}

void
Profile::SetAirspaceBorderColor(ProfileMap &map,
                                unsigned i, const RGB8Color &color)
{
  char name[64];
  MakeAirspaceSettingName(name, "AirspaceBorderColor", i);
  map.SetColor(name, color);
}

void
Profile::SetAirspaceFillColor(ProfileMap &map,
                              unsigned i, const RGB8Color &color)
{
  char name[64];
  MakeAirspaceSettingName(name, "AirspaceFillColor", i);
  map.SetColor(name, color);
}

void
Profile::SetAirspaceFillMode(ProfileMap &map, unsigned i, uint8_t mode)
{
  char name[64];
  MakeAirspaceSettingName(name, "AirspaceFillMode", i);
  map.SetEnum(name, (AirspaceClassRendererSettings::FillMode)mode);
}

void
Profile::SetAirspaceBrush(ProfileMap &map, unsigned i, int brush_index)
{
  char name[64];
  MakeAirspaceSettingName(name, "Brush", i);
  map.Set(name, brush_index);
}
