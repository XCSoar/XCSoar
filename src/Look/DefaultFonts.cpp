/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Screen/Font.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

#include <algorithm>

#include <string.h>

static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, UPixelScalar height,
                  bool bold = false, bool italic = false,
                  bool variable_pitch = true)
{
  memset((char *)font, 0, sizeof(LOGFONT));

  _tcscpy(font->lfFaceName, facename);

  font->lfPitchAndFamily = (variable_pitch ? VARIABLE_PITCH : FIXED_PITCH)
                          | FF_DONTCARE;

  font->lfHeight = (long)height;
  font->lfWeight = (long)(bold ? FW_BOLD : FW_MEDIUM);
  font->lfItalic = italic;

#ifdef WIN32
  if (IsAltair())
    font->lfQuality = NONANTIALIASED_QUALITY;
  else
    font->lfQuality = ANTIALIASED_QUALITY;
#endif
}

static void
LoadAltairLogFonts(FontSettings &settings)
{
  InitialiseLogfont(&settings.infobox,
                    _T("RasterGothicTwentyFourCond"), 24, true);
  InitialiseLogfont(&settings.title, _T("RasterGothicNineCond"), 10);
  InitialiseLogfont(&settings.cdi, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&settings.map_label, _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&settings.map_label_important,
                    _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&settings.map, _T("RasterGothicFourteenCond"), 15);
  InitialiseLogfont(&settings.map_bold,
                    _T("RasterGothicFourteenCond"), 15, true);
  InitialiseLogfont(&settings.infobox_small,
                    _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&settings.monospace, GetStandardMonospaceFontFace(),
                    10, false, false, false);
}

static void
InitialiseLogFonts(FontSettings &settings)
{
  if (IsAltair()) {
    LoadAltairLogFonts(settings);
    return;
  }

#ifndef USE_GDI
  UPixelScalar font_height = Layout::SmallScale((IsAndroid()||IsKobo()) ? 30 : 24);
#else
  UPixelScalar font_height = Layout::SmallScale(35);
#endif

  InitialiseLogfont(&settings.dialog, GetStandardFontFace(), font_height / 2);

  InitialiseLogfont(&settings.infobox, GetStandardFontFace(),
                    font_height, true);

#ifdef WIN32
  settings.infobox.lfCharSet = ANSI_CHARSET;
#endif

  /* the "small" font is derived from the regular font */
  settings.infobox_small = settings.infobox;
  settings.infobox_small.lfHeight = settings.infobox_small.lfHeight * 4 / 5;
  settings.infobox_small.lfWeight = FW_MEDIUM;

  InitialiseLogfont(&settings.title, GetStandardFontFace(), font_height / 3);

  // new font for CDI Scale
  InitialiseLogfont(&settings.cdi, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.6), false, false, false);

  // new font for map labels
  InitialiseLogfont(&settings.map_label, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.39), false, true);

  // new font for map labels big/medium cities
  InitialiseLogfont(&settings.map_label_important, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.39), false, true);

  // new font for map labels
  InitialiseLogfont(&settings.map, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.507));

  // Font for map bold text
  InitialiseLogfont(&settings.map_bold, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.507), true);

#ifndef GNAV
  InitialiseLogfont(&settings.infobox_units, GetStandardFontFace(),
                    (int)(font_height * 0.56));
#endif

  InitialiseLogfont(&settings.monospace, GetStandardMonospaceFontFace(),
                    UPixelScalar(font_height * 0.39), false, false, false);
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
  default_settings = GetDefaults();
  effective_settings = default_settings;

  return Load(effective_settings);
}

void
Fonts::SizeInfoboxFont(unsigned control_width)
{
  AutoSizeInfoBoxFonts(default_settings, control_width);
  AutoSizeInfoBoxFonts(effective_settings, control_width);

  infobox.Load(effective_settings.infobox);
  infobox_small.Load(effective_settings.infobox_small);
#ifndef GNAV
  infobox_units.Load(effective_settings.infobox_units);
#endif

#ifdef ENABLE_OPENGL
  TextCache::Flush();
#endif
}
