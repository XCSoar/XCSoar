/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "LogFile.hpp"
#include "UtilsFont.hpp"
#include "InfoBoxLayout.hpp"
#include "ButtonLabel.hpp"
#include "Profile.hpp"
#include "Asset.hpp"
#include "Screen/Layout.hpp"
#include "Screen/VirtualCanvas.hpp"
#include "Appearance.hpp"
#include "InfoBoxLayout.hpp"

#include <stdio.h>

/// values inside infoboxes  like numbers, etc.
Font InfoWindowFont;
/// Titles of infoboxes like Next, WP L/D etc.
Font TitleWindowFont;
/// text names on the map
Font MapWindowFont;
Font TitleSmallWindowFont;
/// menu buttons, waypoint selection, messages, etc.
Font MapWindowBoldFont;
/// vario display, runway informations
Font CDIWindowFont; // New
/// Flarm Traffic draweing and stats, map labels in italic
Font MapLabelFont;
Font StatisticsFont;

// these are the non-custom parameters
LOGFONT InfoWindowLogFont;
LOGFONT TitleWindowLogFont;
LOGFONT MapWindowLogFont;
LOGFONT TitleSmallWindowLogFont;
LOGFONT MapWindowBoldLogFont;
LOGFONT CDIWindowLogFont;
LOGFONT MapLabelLogFont;
LOGFONT StatisticsLogFont;

#ifndef ENABLE_SDL

static bool
IsNullLogFont(LOGFONT logfont)
{
  LOGFONT LogFontBlank;
  memset((char *)&LogFontBlank, 0, sizeof(LOGFONT));

  if (memcmp(&logfont, &LogFontBlank, sizeof(LOGFONT)) == 0)
    return true;

  return false;
}

#endif /* !ENABLE_SDL */

static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, bool variable_pitch,
                  int height, int width, int bold, bool italic)
{
#ifndef ENABLE_SDL
  memset((char *)font, 0, sizeof(LOGFONT));

  _tcscpy(font->lfFaceName, facename);
  font->lfPitchAndFamily = (variable_pitch ? VARIABLE_PITCH : FIXED_PITCH)
                          | FF_DONTCARE;
  font->lfHeight = (long)height;
  font->lfWidth = (long)width;
  font->lfWeight = (long)bold;
  font->lfItalic = italic;
  font->lfQuality = ANTIALIASED_QUALITY;
#endif /* !ENABLE_SDL */
}

static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, bool variable_pitch,
                  int height, int width, bool bold, bool italic)
{
#ifndef ENABLE_SDL
  InitialiseLogfont(font, facename, variable_pitch, height, width,
                    (bold ? FW_BOLD : FW_MEDIUM), italic);
#endif /* !ENABLE_SDL */
}

void
InitializeFont(Font *theFont, LOGFONT autoLogFont,
                  LOGFONT * LogFontUsed)
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  if (theFont->defined() || IsNullLogFont(autoLogFont))
    return;

  if (theFont->set(&autoLogFont) && LogFontUsed != NULL)
    *LogFontUsed = autoLogFont; // RLD save for custom font GUI
#endif /* !ENABLE_SDL */
}

void
LoadCustomFont(Font *theFont, const TCHAR FontRegKey[], LOGFONT * LogFontUsed)
{
#ifdef ENABLE_SDL
  // XXX
#else /* !ENABLE_SDL */
  LOGFONT logfont;
  memset((char *)&logfont, 0, sizeof(LOGFONT));
  if (!Profile::GetFont(FontRegKey, &logfont))
    return;

  if (theFont->set(&logfont) && LogFontUsed != NULL)
    *LogFontUsed = logfont; // RLD save for custom font GUI
#endif /* !ENABLE_SDL */
}

static void
InitialiseFontsPNA(const struct Appearance &appearance, RECT rc)
{
  if (!is_pna())
    return;

  int ScreenSize = 0;

  int iWidth = rc.right - rc.left;
  int iHeight = rc.bottom - rc.top;

  if (iWidth == 240 && iHeight == 320)
    ScreenSize = (ScreenSize_t)ss240x320; // QVGA portrait
  else if (iWidth == 480 && iHeight == 640)
    ScreenSize = (ScreenSize_t)ss480x640; //  VGA
  else if (iWidth == 480 && iHeight == 800)
    ScreenSize = (ScreenSize_t)ss480x800;
  else if (iWidth == 480 && iHeight == 272)
    ScreenSize = (ScreenSize_t)ss480x272; // WQVGA  landscape
  else if (iWidth == 320 && iHeight == 240)
    ScreenSize = (ScreenSize_t)ss320x240; //  QVGA
  else if (iWidth == 480 && iHeight == 234)
    ScreenSize = (ScreenSize_t)ss480x234; //   iGo
  else if (iWidth == 640 && iHeight == 480)
    ScreenSize = (ScreenSize_t)ss640x480; //   VGA
  else if (iWidth == 800 && iHeight == 480)
    ScreenSize = (ScreenSize_t)ss800x480; //  WVGA
  else {
    LogStartUp(_T("--- ERROR UNKNOWN RESOLUTION %dx%d !"), iWidth, iHeight);
    return;
  }

  if (ScreenSize == (ScreenSize_t)ss480x272) {
    // WQVGA  e.g. MIO
    InitialiseLogfont(&InfoWindowLogFont, _T("TahomaBD"),
                      true, 28, 0, 800, false);
    InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"),
                      true, 16, 0, 500, false);
    InitialiseLogfont(&TitleSmallWindowLogFont, _T("Tahoma"),
                      true, 16, 0, 100, true);
    InitialiseLogfont(&CDIWindowLogFont, _T("TahomaBD"),
                      true, 28, 0, 400, false);
    InitialiseLogfont(&MapLabelLogFont, _T("Tahoma"),
                      true, 14, 0, 100, true);
    InitialiseLogfont(&StatisticsLogFont, _T("Tahoma"),
                      true, 20, 0, 400, false);
    InitialiseLogfont(&MapWindowLogFont, _T("Tahoma"),
                      true, 18, 0, 400, false);
    InitialiseLogfont(&MapWindowBoldLogFont, _T("TahomaBD"),
                      true, 16, 0, 500, false);

    if (InfoBoxLayout::InfoBoxGeometry == InfoBoxLayout::ibRight8)
      // We don't use vario gauge in landscape geo5 anymore.. but doesn't hurt.
      SetGlobalEllipse(1.32f);
    else
      SetGlobalEllipse(1.1f);
  } else if (ScreenSize == (ScreenSize_t)ss480x234) {
    // e.g. Messada 2440
    InitialiseLogfont(&InfoWindowLogFont, _T("TahomaBD"),
                      true, 22, 0, 400, false);
    InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"),
                      true, 18, 0, 500, false);
    InitialiseLogfont(&TitleSmallWindowLogFont, _T("Tahoma"),
                      true, 20, 0, 400, true);
    InitialiseLogfont(&CDIWindowLogFont, _T("TahomaBD"),
                      true, 28, 0, 400, false);
    InitialiseLogfont(&MapLabelLogFont, _T("Tahoma"),
                      true, 14, 0, 100, true);
    InitialiseLogfont(&StatisticsLogFont, _T("Tahoma"),
                      true, 20, 0, 400, false);
    InitialiseLogfont(&MapWindowLogFont, _T("Tahoma"),
                      true, 18, 0, 400, false);
    InitialiseLogfont(&MapWindowBoldLogFont, _T("TahomaBD"),
                      true, 16, 0, 500, false);

    SetGlobalEllipse(1.1f); // to be checked, TODO
  } else if (ScreenSize == (ScreenSize_t)ss800x480) {// e.g. ipaq 31x {
    switch (InfoBoxLayout::InfoBoxGeometry) {
    case InfoBoxLayout::ibTop4Bottom4:
    case InfoBoxLayout::ibBottom8:
    case InfoBoxLayout::ibTop8:
    case InfoBoxLayout::ibLeft4Right4:
    case InfoBoxLayout::ibGNav: // standard landscape
      InitialiseLogfont(&InfoWindowLogFont, _T("TahomaBD"),
                        true, 56, 0, 600, false);
      InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"),
                        true, 20, 0, 200, false);
      SetGlobalEllipse(1.1f);
      break;
    case InfoBoxLayout::ibLeft8:
    case InfoBoxLayout::ibRight8:
      InitialiseLogfont(&InfoWindowLogFont, _T("TahomaBD"),
                        true, 64, 0, 600, false);
      InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"),
                        true, 26, 0, 600, false);
      SetGlobalEllipse(1.32f);
      break;
    case InfoBoxLayout::ibSquare:
      InitialiseLogfont(&InfoWindowLogFont, _T("TahomaBD"),
                        true, 66, 0, 600, false);
      InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"),
                        true, 23, 0, 400, false);
      break;

      // This is a failsafe with an impossible setting so that you know
      // something is going very wrong.
    default:
      InitialiseLogfont(&InfoWindowLogFont, _T("TahomaBD"),
                        true, 30, 0, 600, false);
      InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"),
                        true, 10, 0, 200, false);
      break;
    } // special geometry cases for 31x

    InitialiseLogfont(&TitleSmallWindowLogFont, _T("Tahoma"),
                      true, 16, 0, 100, true);
    InitialiseLogfont(&CDIWindowLogFont, _T("Tahoma"),
                      true, 36, 0, 400, false);
    InitialiseLogfont(&MapLabelLogFont, _T("Tahoma"),
                      true, 28, 0, 100, true);
    InitialiseLogfont(&StatisticsLogFont, _T("Tahoma"),
                      true, 48, 0, 400, false);
    InitialiseLogfont(&MapWindowLogFont, _T("Tahoma"),
                      true, 36, 0, 400, false);
    InitialiseLogfont(&MapWindowBoldLogFont, _T("TahomaBD"),
                      true, 32, 0, 600, false);
  }
}

static void
InitialiseFontsAltair()
{
  if (!is_altair())
    return;

  InitialiseLogfont(&InfoWindowLogFont, _T("RasterGothicTwentyFourCond"),
                    true, 24, 0, 700, false);
  InitialiseLogfont(&TitleWindowLogFont, _T("RasterGothicNineCond"),
                    true, 10, 0, 500, false);
  InitialiseLogfont(&CDIWindowLogFont, _T("RasterGothicEighteenCond"),
                    true, 19, 0, 700, false);
  InitialiseLogfont(&MapLabelLogFont, _T("RasterGothicTwelveCond"),
                    true, 13, 0, 500, false);
  InitialiseLogfont(&StatisticsLogFont, _T("RasterGothicFourteenCond"),
                    true, 15, 0, 500, false);
  InitialiseLogfont(&MapWindowLogFont, _T("RasterGothicFourteenCond"),
                    true, 15, 0, 500, false);
  InitialiseLogfont(&MapWindowBoldLogFont, _T("RasterGothicFourteenCond"),
                    true, 15, 0, 700, false);
  InitialiseLogfont(&TitleSmallWindowLogFont, _T("RasterGothicEighteenCond"),
                    true, 19, 0, 700, false);
}

static void
InitialiseLogFonts()
{
  int FontHeight = (Layout::square ? Layout::Scale(26) : Layout::Scale(35));

#ifdef WINDOWSPC
  FontHeight = (int)(FontHeight / 1.35);
#endif

#ifndef ENABLE_SDL
  // oversize first so can then scale down
  int iFontHeight = (int)(FontHeight * 1.4);

  LOGFONT logfont;
  InitialiseLogfont(&logfont,
                    (!is_pna() ? _T("Tahoma") : _T("DejaVu Sans Condensed")),
                    true, iFontHeight, 0, true, false);
  logfont.lfCharSet = ANSI_CHARSET;

  // JMW algorithm to auto-size info window font.
  // this is still required in case title font property doesn't exist.
  VirtualCanvas canvas(1, 1);
  SIZE tsize;
  do {
    HFONT TempWindowFont;
    HFONT hfOld;

    iFontHeight--;
    logfont.lfHeight = iFontHeight;

    TempWindowFont = CreateFontIndirect(&logfont);
    hfOld = (HFONT)SelectObject(canvas, TempWindowFont);

    tsize = canvas.text_size(_T("1234m"));
    // unselect it before deleting it
    SelectObject(canvas, hfOld);
    DeleteObject(TempWindowFont);
  } while (tsize.cx > InfoBoxLayout::ControlWidth);

  iFontHeight++;
  logfont.lfHeight = iFontHeight;
  memset(&InfoWindowLogFont, 0, sizeof(LOGFONT));
  memcpy(&InfoWindowLogFont, &logfont, sizeof(LOGFONT));

#else /* !ENABLE_SDL */
  // XXX implement
#endif /* !ENABLE_SDL */

  InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * 0.333), 0, true, false);

  // new font for CDI Scale
  InitialiseLogfont(&CDIWindowLogFont, _T("Tahoma"), false,
                    (int)(FontHeight * 0.6), 0, false, false);

  // new font for map labels
  InitialiseLogfont(&MapLabelLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * 0.39), 0, false, true);

  // Font for map other text
  InitialiseLogfont(&StatisticsLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * 0.7), 0, false, false);

  // new font for map labels
  InitialiseLogfont(&MapWindowLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * 0.507), 0, false, false);

  // Font for map bold text
  InitialiseLogfont(&MapWindowBoldLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * 0.507), 0, true, false);

  InitialiseLogfont(&TitleSmallWindowLogFont, _T("Tahoma"), true,
                    Layout::Scale(20), Layout::Scale(8), false, false);
}

void
InitialiseFonts(const struct Appearance &appearance, RECT rc)
{
  //this routine must be called only at start/restart of XCSoar b/c there are many pointers to these fonts
  ResetFonts();

  InitialiseLogFonts();

  InitialiseFontsPNA(appearance, rc);
  InitialiseFontsAltair();

  InitializeFont(&InfoWindowFont, InfoWindowLogFont);
  InitializeFont(&TitleWindowFont, TitleWindowLogFont);
  InitializeFont(&CDIWindowFont, CDIWindowLogFont);
  InitializeFont(&MapLabelFont, MapLabelLogFont);
  InitializeFont(&StatisticsFont, StatisticsLogFont);
  InitializeFont(&MapWindowFont, MapWindowLogFont);
  InitializeFont(&MapWindowBoldFont, MapWindowBoldLogFont);
  InitializeFont(&TitleSmallWindowFont, TitleSmallWindowLogFont);

  if (appearance.UseCustomFonts) {
    LoadCustomFont(&InfoWindowFont, szProfileFontInfoWindowFont);
    LoadCustomFont(&TitleWindowFont, szProfileFontTitleWindowFont);
    LoadCustomFont(&CDIWindowFont, szProfileFontCDIWindowFont);
    LoadCustomFont(&MapLabelFont, szProfileFontMapLabelFont);
    LoadCustomFont(&StatisticsFont, szProfileFontStatisticsFont);
    LoadCustomFont(&MapWindowFont, szProfileFontMapWindowFont);
    LoadCustomFont(&MapWindowBoldFont, szProfileFontMapWindowBoldFont);
    LoadCustomFont(&TitleSmallWindowFont, szProfileFontTitleSmallWindowFont);
  }
}

void
ResetFonts()
{
  InfoWindowFont.reset();
  TitleWindowFont.reset();
  CDIWindowFont.reset();
  MapLabelFont.reset();
  MapWindowFont.reset();
  MapWindowBoldFont.reset();
  StatisticsFont.reset();
  TitleSmallWindowFont.reset();
}
