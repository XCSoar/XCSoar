/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "DefaultFonts.hpp"
#include "AutoFont.hpp"
#include "GlobalFonts.hpp"
#include "FontSettings.hpp"
#include "StandardFonts.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Screen/Font.hpp"
#include "Screen/Layout.hpp"

#ifdef HAVE_TEXT_CACHE
#include "Screen/Custom/Cache.hpp"
#endif

#include <algorithm>

#include <string.h>

#ifdef WIN32

static void
LoadAltairLogFonts(FontSettings &settings)
{
  settings.dialog = FontDescription(_T("RasterGothicTwelveCond"), 13);
#ifdef GNAV
  settings.dialog_small = FontDescription(_T("RasterGothicNineCond"), 10);
#endif
  settings.infobox = FontDescription(_T("RasterGothicTwentyFourCond"),
                                     24, true);
  settings.title = FontDescription(_T("RasterGothicNineCond"), 10);
  settings.cdi = FontDescription(_T("RasterGothicEighteenCond"), 19, true);
  settings.map_label = FontDescription(_T("RasterGothicTwelveCond"), 13);
  settings.map_label_important = FontDescription(_T("RasterGothicTwelveCond"),
                                                 13);
  settings.map = FontDescription(_T("RasterGothicFourteenCond"), 15);
  settings.map_bold = FontDescription(_T("RasterGothicFourteenCond"),
                                      15, true);
  settings.infobox_small = FontDescription(_T("RasterGothicEighteenCond"),
                                           19, true);
  settings.monospace = FontDescription(GetStandardMonospaceFontFace(),
                                       12, false, false, true);
}

#endif

static void
InitialiseLogFonts(FontSettings &settings)
{
#ifdef WIN32
  if (IsAltair()) {
    LoadAltairLogFonts(settings);
    return;
  }
#endif

  settings.dialog = FontDescription(std::min(Layout::FontScale(12),
                                             Layout::min_screen_pixels / 20));

  settings.infobox = FontDescription(Layout::FontScale(30), true);

  /* the "small" font is derived from the regular font */
  settings.infobox_small = settings.infobox;
  settings.infobox_small.SetHeight(settings.infobox_small.GetHeight() * 4 / 5);
  settings.infobox_small.SetBold(false);

  settings.title = FontDescription(Layout::FontScale(8));

  // new font for CDI Scale
  settings.cdi = FontDescription(Layout::FontScale(10),
                                 false, false, true);

  // new font for map labels
  settings.map_label = FontDescription(Layout::FontScale(8), false, true);

  // new font for map labels big/medium cities
  settings.map_label_important = FontDescription(Layout::FontScale(9),
                                                 false, true);

  // new font for map labels
  settings.map = FontDescription(Layout::FontScale(10));

  // Font for map bold text
  settings.map_bold = FontDescription(Layout::FontScale(10), true);

#ifndef GNAV
  settings.infobox_units = FontDescription(Layout::FontScale(9));
#endif

  settings.monospace = FontDescription(settings.dialog.GetHeight(),
                                       false, false, true);
}

FontSettings
Fonts::GetDefaults()
{
  FontSettings settings;
  InitialiseLogFonts(settings);
  return settings;
}

bool
Fonts::Initialize()
{
  const auto default_settings = GetDefaults();

  return Load(default_settings);
}

void
Fonts::SizeInfoboxFont(unsigned control_width)
{
  auto default_settings = GetDefaults();

  AutoSizeInfoBoxFonts(default_settings, control_width);

  infobox.Load(default_settings.infobox);
  infobox_small.Load(default_settings.infobox_small);
#ifndef GNAV
  infobox_units.Load(default_settings.infobox_units);
#endif

#ifdef HAVE_TEXT_CACHE
  TextCache::Flush();
#endif
}
