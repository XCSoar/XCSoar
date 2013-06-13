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
#include "Fonts.hpp"
#include "StandardFonts.hpp"
#include "Screen/Font.hpp"
#include "Screen/Layout.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Cache.hpp"
#endif

#include <algorithm>

#include <string.h>

// these are the non-custom parameters
LOGFONT log_infobox;
#ifndef GNAV
static LOGFONT log_infobox_units;
#endif
LOGFONT log_title;
LOGFONT log_map;
LOGFONT log_infobox_small;
LOGFONT log_map_bold;
LOGFONT log_cdi;
LOGFONT log_map_label;
LOGFONT log_map_label_important;
static LOGFONT log_monospace;

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
LoadAltairLogFonts()
{
  InitialiseLogfont(&log_infobox, _T("RasterGothicTwentyFourCond"), 24, true);
  InitialiseLogfont(&log_title, _T("RasterGothicNineCond"), 10);
  InitialiseLogfont(&log_cdi, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&log_map_label, _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&log_map_label_important,
                    _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&log_map, _T("RasterGothicFourteenCond"), 15);
  InitialiseLogfont(&log_map_bold, _T("RasterGothicFourteenCond"), 15, true);
  InitialiseLogfont(&log_infobox_small, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&log_monospace, GetStandardMonospaceFontFace(),
                    10, false, false, false);
}

static void
InitialiseLogFonts()
{
  if (IsAltair()) {
    LoadAltairLogFonts();
    return;
  }

#ifndef USE_GDI
  UPixelScalar font_height = Layout::SmallScale((IsAndroid()||IsKobo()) ? 30 : 24);
#else
  UPixelScalar font_height = Layout::SmallScale(35);
#endif

  // oversize first so can then scale down
  InitialiseLogfont(&log_infobox, GetStandardFontFace(),
                    (int)(font_height * 1.4), true, false, true);

#ifdef WIN32
  log_infobox.lfCharSet = ANSI_CHARSET;
#endif

  InitialiseLogfont(&log_title, GetStandardFontFace(), font_height / 3);

  // new font for CDI Scale
  InitialiseLogfont(&log_cdi, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.6), false, false, false);

  // new font for map labels
  InitialiseLogfont(&log_map_label, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.39), false, true);

  // new font for map labels big/medium cities
  InitialiseLogfont(&log_map_label_important, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.39), false, true);

  // new font for map labels
  InitialiseLogfont(&log_map, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.507));

  // Font for map bold text
  InitialiseLogfont(&log_map_bold, GetStandardFontFace(),
                    UPixelScalar(font_height * 0.507), true);

  InitialiseLogfont(&log_infobox_small, GetStandardFontFace(),
                    Layout::Scale(20));

#ifndef GNAV
  InitialiseLogfont(&log_infobox_units, GetStandardFontFace(),
                    (int)(font_height * 0.56));
#endif

  InitialiseLogfont(&log_monospace, GetStandardMonospaceFontFace(),
                    UPixelScalar(font_height * 0.39), false, false, false);
}

bool
Fonts::Initialize()
{
  InitialiseLogFonts();

  title.Load(log_title);
  cdi.Load(log_cdi);
  map_label.Load(log_map_label);
  map_label_important.Load(log_map_label_important);
  map.Load(log_map);
  map_bold.Load(log_map_bold);
  monospace.Load(log_monospace);

  return title.IsDefined() && cdi.IsDefined() &&
    map_label.IsDefined() && map_label_important.IsDefined() &&
    map.IsDefined() && map_bold.IsDefined() &&
    monospace.IsDefined();
}

void
Fonts::SizeInfoboxFont(unsigned control_width)
{
  if (!IsAltair())
    AutoSizeFont(log_infobox, control_width, _T("1234m"));
  infobox.Load(log_infobox);

#ifndef GNAV
  LOGFONT lf = log_infobox;
  unsigned height = lf.lfHeight;
  lf = log_infobox_units;
  lf.lfHeight = (height * 2) / 5;
  infobox_units.Load(lf);
#endif

  if (!IsAltair())
    AutoSizeFont(log_infobox_small, control_width, _T("12345m"));
  infobox_small.Load(log_infobox_small);

#ifdef ENABLE_OPENGL
  TextCache::Flush();
#endif
}
