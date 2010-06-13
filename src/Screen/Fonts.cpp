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
    GetFontFromString(_T("28,0,0,0,800,0,0,0,0,0,0,3,2,TahomaBD"), &InfoWindowLogFont);
    GetFontFromString(_T("16,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), &TitleWindowLogFont);
    GetFontFromString(_T("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &TitleSmallWindowLogFont);
    GetFontFromString(_T("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), &CDIWindowLogFont);
    GetFontFromString(_T("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &MapLabelLogFont); // RLD 16 works well too
    GetFontFromString(_T("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &StatisticsLogFont);//  (RLD is this used?)
    GetFontFromString(_T("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &MapWindowLogFont);
    GetFontFromString(_T("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), &MapWindowBoldLogFont);
    if (appearance.InfoBoxGeom == 5)
      // We don't use vario gauge in landscape geo5 anymore.. but doesn't hurt.
      SetGlobalEllipse(1.32f);
    else
      SetGlobalEllipse(1.1f);
  } else if (ScreenSize == (ScreenSize_t)ss480x234) {
    // e.g. Messada 2440
    GetFontFromString(_T("22,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), &InfoWindowLogFont);
    GetFontFromString(_T("18,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), &TitleWindowLogFont);
    GetFontFromString(_T("20,0,0,0,400,1,0,0,0,0,0,3,2,Tahoma"), &TitleSmallWindowLogFont);
    GetFontFromString(_T("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), &CDIWindowLogFont);
    GetFontFromString(_T("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &MapLabelLogFont); // RLD 16 works well too
    GetFontFromString(_T("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &StatisticsLogFont);//  (RLD is this used?)
    GetFontFromString(_T("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &MapWindowLogFont);
    GetFontFromString(_T("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), &MapWindowBoldLogFont);
    SetGlobalEllipse(1.1f); // to be checked, TODO
  } else if (ScreenSize == (ScreenSize_t)ss800x480) {// e.g. ipaq 31x {
    switch (appearance.InfoBoxGeom) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 6: // standard landscape
      GetFontFromString(_T("56,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &InfoWindowLogFont);
      GetFontFromString(_T("20,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), &TitleWindowLogFont);
      SetGlobalEllipse(1.1f);
      break;
    case 4:
    case 5:
      GetFontFromString(_T("64,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &InfoWindowLogFont);
      GetFontFromString(_T("26,0,0,0,600,0,0,0,0,0,0,3,2,Tahoma"), &TitleWindowLogFont);
      SetGlobalEllipse(1.32f);
      break;
    case 7:
      GetFontFromString(_T("66,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &InfoWindowLogFont);
      GetFontFromString(_T("23,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &TitleWindowLogFont);
      break;

      // This is a failsafe with an impossible setting so that you know
      // something is going very wrong.
    default:
      GetFontFromString(_T("30,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &InfoWindowLogFont);
      GetFontFromString(_T("10,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), &TitleWindowLogFont);
      break;
    } // special geometry cases for 31x

    GetFontFromString(_T("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &TitleSmallWindowLogFont);
    GetFontFromString(_T("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &CDIWindowLogFont);
    GetFontFromString(_T("28,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &MapLabelLogFont);
    GetFontFromString(_T("48,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &StatisticsLogFont);
    GetFontFromString(_T("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &MapWindowLogFont);
    GetFontFromString(_T("32,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &MapWindowBoldLogFont);
  }
}

static void
InitialiseFontsAltair()
{
  if (!is_altair())
    return;

  // RLD Altair also loads these in registry and by default, uses the registry

  GetFontFromString(_T("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"), &InfoWindowLogFont);
  GetFontFromString(_T("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"), &TitleWindowLogFont);
  GetFontFromString(_T("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), &CDIWindowLogFont);
  GetFontFromString(_T("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"), &MapLabelLogFont);
  GetFontFromString(_T("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), &StatisticsLogFont);
  GetFontFromString(_T("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), &MapWindowLogFont);
  GetFontFromString(_T("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), &MapWindowBoldLogFont);
  GetFontFromString(_T("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), &TitleSmallWindowLogFont);
}

#ifndef ENABLE_SDL
static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, bool variable_pitch,
                  long height, long width, bool bold, bool italic) {
  memset((char *)font, 0, sizeof(LOGFONT));

  _tcscpy(font->lfFaceName, facename);
  font->lfPitchAndFamily = (variable_pitch ? VARIABLE_PITCH : FIXED_PITCH)
                          | FF_DONTCARE;
  font->lfHeight = height;
  font->lfWidth = width;
  font->lfWeight = (bold ? FW_BOLD : FW_MEDIUM);
  font->lfItalic = italic;
  font->lfQuality = ANTIALIASED_QUALITY;
}
#endif

static void
InitialiseLogFonts()
{
#ifndef ENABLE_SDL
  int FontHeight;

  if (Layout::square)
    // square
    FontHeight = Layout::Scale(26);
  else
    // portrait & landscape
    FontHeight = Layout::Scale(35);

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

#ifdef WINDOWSPC
  FontHeight = (int)(FontHeight / 1.35);
#endif

  InitialiseLogfont(&TitleWindowLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * TITLEFONTHEIGHTRATIO), 0, true, false);

  // new font for CDI Scale
  InitialiseLogfont(&CDIWindowLogFont, _T("Tahoma"), false,
                    (int)(FontHeight * CDIFONTHEIGHTRATIO), 0, false, false);

  // new font for map labels
  InitialiseLogfont(&MapLabelLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * MAPFONTHEIGHTRATIO), 0, false, true);

  // Font for map other text
  InitialiseLogfont(&StatisticsLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * STATISTICSFONTHEIGHTRATIO),
                    0, false, false);

  // new font for map labels
  InitialiseLogfont(&MapWindowLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * MAPFONTHEIGHTRATIO * 1.3),
                    0, false, false);

  // Font for map bold text
  InitialiseLogfont(&MapWindowBoldLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * MAPFONTHEIGHTRATIO * 1.3),
                    0, true, false);

  InitialiseLogfont(&TitleSmallWindowLogFont, _T("Tahoma"), true,
                    Layout::Scale(20), Layout::Scale(8), false, false);
#else /* !ENABLE_SDL */
  // XXX implement
#endif /* !ENABLE_SDL */
}

void
InitialiseFonts(const struct Appearance &appearance, RECT rc)
{
  //this routine must be called only at start/restart of XCSoar b/c there are many pointers to these fonts
  ResetFonts();

  InitialiseLogFonts();

  InitialiseFontsPNA(appearance, rc);
  InitialiseFontsAltair();

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

  InitializeFont(&InfoWindowFont, InfoWindowLogFont);
  InitializeFont(&TitleWindowFont, TitleWindowLogFont);
  InitializeFont(&CDIWindowFont, CDIWindowLogFont);
  InitializeFont(&MapLabelFont, MapLabelLogFont);
  InitializeFont(&StatisticsFont, StatisticsLogFont);
  InitializeFont(&MapWindowFont, MapWindowLogFont);
  InitializeFont(&MapWindowBoldFont, MapWindowBoldLogFont);
  InitializeFont(&TitleSmallWindowFont, TitleSmallWindowLogFont);
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
