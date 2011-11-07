/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/AnyCanvas.hpp"

/// values inside infoboxes  like numbers, etc.
Font Fonts::InfoBox;
Font Fonts::InfoBoxSmall;
#ifndef GNAV
Font Fonts::InfoBoxUnits;
#endif
/// Titles of infoboxes like Next, WP L/D etc.
Font Fonts::Title;
/// text names on the map
Font Fonts::Map;
/// menu buttons, waypoint selection, messages, etc.
Font Fonts::MapBold;
/// vario display, runway informations
Font Fonts::CDI;
/// Flarm Traffic draweing and stats, map labels in italic
Font Fonts::MapLabel;
/// font labels for important labels (e.g. big/medium cities)
Font Fonts::MapLabelImportant;
Font Fonts::monospace;

// these are the non-custom parameters
LOGFONT LogInfoBox;
#ifndef GNAV
LOGFONT LogInfoBoxUnits;
#endif
LOGFONT LogTitle;
LOGFONT LogMap;
LOGFONT LogInfoBoxSmall;
LOGFONT LogMapBold;
LOGFONT LogCDI;
LOGFONT LogMapLabel;
LOGFONT LogMapLabelImportant;
static LOGFONT log_monospace;

static const TCHAR *
GetStandardMonospaceFontFace()
{
  if (is_android())
    return _T("Droid Sans Mono");

  return _T("Courier");
}

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
  if (is_altair())
    font->lfQuality = NONANTIALIASED_QUALITY;
  else
    font->lfQuality = ANTIALIASED_QUALITY;
#endif
}

static void
LoadAltairLogFonts()
{
  InitialiseLogfont(&LogInfoBox, _T("RasterGothicTwentyFourCond"), 24, true);
  InitialiseLogfont(&LogTitle, _T("RasterGothicNineCond"), 10);
  InitialiseLogfont(&LogCDI, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&LogMapLabel, _T("RasterGothicTwelveCond"), 13);
  InitialiseLogfont(&LogMapLabelImportant,
                    _T("RasterGothicTwelveCond"), 13, true);
  InitialiseLogfont(&LogMap, _T("RasterGothicFourteenCond"), 15);
  InitialiseLogfont(&LogMapBold, _T("RasterGothicFourteenCond"), 15, true);
  InitialiseLogfont(&LogInfoBoxSmall, _T("RasterGothicEighteenCond"), 19, true);
  InitialiseLogfont(&log_monospace, GetStandardMonospaceFontFace(),
                    10, false, false, false);
}

static void
SizeLogFont(LOGFONT &logfont, UPixelScalar width, const TCHAR* str)
{
  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.
  AnyCanvas canvas;
  PixelSize tsize;
  do {
    --logfont.lfHeight;

    Font font;
    if (!font.Set(logfont))
      break;

    canvas.select(font);
    tsize = canvas.text_size(str);
  } while ((unsigned)tsize.cx > width);

  ++logfont.lfHeight;
}

static void
InitialiseLogFonts()
{
  if (is_altair()) {
    LoadAltairLogFonts();
    return;
  }

#ifndef USE_GDI
  UPixelScalar FontHeight = Layout::SmallScale(is_android() ? 30 : 24);
#else
  UPixelScalar FontHeight = Layout::SmallScale(35);
#endif

  // oversize first so can then scale down
  InitialiseLogfont(&LogInfoBox, Fonts::GetStandardFontFace(),
                    (int)(FontHeight * 1.4), true, false, true);

#ifdef WIN32
  LogInfoBox.lfCharSet = ANSI_CHARSET;
#endif

  InitialiseLogfont(&LogTitle, Fonts::GetStandardFontFace(),
                    FontHeight / 3, true);

  // new font for CDI Scale
  InitialiseLogfont(&LogCDI, Fonts::GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.6), false, false, false);

  // new font for map labels
  InitialiseLogfont(&LogMapLabel, Fonts::GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.39), false, true);

  // new font for map labels big/medium cities
  InitialiseLogfont(&LogMapLabelImportant, Fonts::GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.39), true, true);

  // new font for map labels
  InitialiseLogfont(&LogMap, Fonts::GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.507));

  // Font for map bold text
  InitialiseLogfont(&LogMapBold, Fonts::GetStandardFontFace(),
                    UPixelScalar(FontHeight * 0.507), true);

  InitialiseLogfont(&LogInfoBoxSmall, Fonts::GetStandardFontFace(),
                    Layout::Scale(20));

  InitialiseLogfont(&LogInfoBoxSmall, Fonts::GetStandardFontFace(),
                    (int)(FontHeight * 0.56), true);

  InitialiseLogfont(&log_monospace, GetStandardMonospaceFontFace(),
                    UPixelScalar(FontHeight * 0.39), false, false, false);
}

bool
Fonts::Initialize()
{
  InitialiseLogFonts();

  Title.Set(LogTitle);
  CDI.Set(LogCDI);
  MapLabel.Set(LogMapLabel);
  MapLabelImportant.Set(LogMapLabelImportant);
  Map.Set(LogMap);
  MapBold.Set(LogMapBold);
  monospace.Set(log_monospace);

  return Title.IsDefined() && CDI.IsDefined() &&
    MapLabel.IsDefined() && MapLabelImportant.IsDefined() &&
    Map.IsDefined() && MapBold.IsDefined() &&
    monospace.IsDefined();
}

void
Fonts::SizeInfoboxFont(UPixelScalar control_width)
{
  LOGFONT lf = LogInfoBox;

  if (!is_altair())
    SizeLogFont(lf, control_width, _T("1234m"));
  InfoBox.Set(lf);

#ifndef GNAV
  unsigned height = lf.lfHeight;
  lf = LogInfoBoxUnits;
  lf.lfHeight = (height * 2) / 5;
  InfoBoxUnits.Set(lf);
#endif

  lf = LogInfoBoxSmall;
  if (!is_altair())
    SizeLogFont(lf, control_width, _T("12345m"));
  InfoBoxSmall.Set(lf);
}

void
Fonts::Deinitialize()
{
  InfoBox.Reset();
  InfoBoxSmall.Reset();
#ifndef GNAV
  InfoBoxUnits.Reset();
#endif
  Title.Reset();
  Map.Reset();
  MapBold.Reset();
  CDI.Reset();
  MapLabel.Reset();
  MapLabelImportant.Reset();
  monospace.Reset();
}

const TCHAR*
Fonts::GetStandardFontFace()
{
  if (is_altair())
    return _T("RasterGothicFourteenCond");

  if (is_android())
    return _T("Droid Sans");

  return _T("Tahoma");
}
