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
LOGFONT autoInfoWindowLogFont;
LOGFONT autoTitleWindowLogFont;
LOGFONT autoMapWindowLogFont;
LOGFONT autoTitleSmallWindowLogFont;
LOGFONT autoMapWindowBoldLogFont;
LOGFONT autoCDIWindowLogFont;
LOGFONT autoMapLabelLogFont;
LOGFONT autoStatisticsLogFont;

#ifndef ENABLE_SDL

static void
ApplyClearType(LOGFONT *logfont)
{
  logfont->lfQuality = ANTIALIASED_QUALITY;
}

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

  ApplyClearType(&autoLogFont);
  theFont->set(&autoLogFont);
  if (theFont->defined() && LogFontUsed != NULL)
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
  propGetFontSettings(FontRegKey, &logfont);
  if (IsNullLogFont(logfont))
    return;

  theFont->set(&logfont);
  if (theFont->defined() && LogFontUsed != NULL)
    *LogFontUsed = logfont; // RLD save for custom font GUI
#endif /* !ENABLE_SDL */
}

static void
InitialiseFontsHardCoded(const struct Appearance &appearance, RECT rc)
{
  if (is_pna()) {
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
      propGetFontSettingsFromString(TEXT("28,0,0,0,800,0,0,0,0,0,0,3,2,TahomaBD"), &autoInfoWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), &autoTitleWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &autoTitleSmallWindowLogFont);
      propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), &autoCDIWindowLogFont);
      propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &autoMapLabelLogFont); // RLD 16 works well too
      propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoStatisticsLogFont);//  (RLD is this used?)
      propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoMapWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), &autoMapWindowBoldLogFont);
      if (appearance.InfoBoxGeom == 5)
        // We don't use vario gauge in landscape geo5 anymore.. but doesn't hurt.
        SetGlobalEllipse(1.32f);
      else
        SetGlobalEllipse(1.1f);
    } else if (ScreenSize == (ScreenSize_t)ss480x234) {
      // e.g. Messada 2440
      propGetFontSettingsFromString(TEXT("22,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), &autoInfoWindowLogFont);
      propGetFontSettingsFromString(TEXT("18,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), &autoTitleWindowLogFont);
      propGetFontSettingsFromString(TEXT("20,0,0,0,400,1,0,0,0,0,0,3,2,Tahoma"), &autoTitleSmallWindowLogFont);
      propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), &autoCDIWindowLogFont);
      propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &autoMapLabelLogFont); // RLD 16 works well too
      propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoStatisticsLogFont);//  (RLD is this used?)
      propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoMapWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), &autoMapWindowBoldLogFont);
      SetGlobalEllipse(1.1f); // to be checked, TODO
    } else if (ScreenSize == (ScreenSize_t)ss800x480) {// e.g. ipaq 31x {
      switch (appearance.InfoBoxGeom) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 6: // standard landscape
        propGetFontSettingsFromString(TEXT("56,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &autoInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("20,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), &autoTitleWindowLogFont);
        SetGlobalEllipse(1.1f);
        break;
      case 4:
      case 5:
        propGetFontSettingsFromString(TEXT("64,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &autoInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("26,0,0,0,600,0,0,0,0,0,0,3,2,Tahoma"), &autoTitleWindowLogFont);
        SetGlobalEllipse(1.32f);
        break;
      case 7:
        propGetFontSettingsFromString(TEXT("66,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &autoInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("23,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoTitleWindowLogFont);
        break;

        // This is a failsafe with an impossible setting so that you know
        // something is going very wrong.
      default:
        propGetFontSettingsFromString(TEXT("30,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &autoInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("10,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), &autoTitleWindowLogFont);
        break;
      } // special geometry cases for 31x

      propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &autoTitleSmallWindowLogFont);
      propGetFontSettingsFromString(TEXT("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoCDIWindowLogFont);
      propGetFontSettingsFromString(TEXT("28,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), &autoMapLabelLogFont);
      propGetFontSettingsFromString(TEXT("48,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoStatisticsLogFont);
      propGetFontSettingsFromString(TEXT("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), &autoMapWindowLogFont);
      propGetFontSettingsFromString(TEXT("32,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), &autoMapWindowBoldLogFont);
    }
  }

  if (is_altair()) {
    // RLD Altair also loads these in registry and by default, uses the registry
    propGetFontSettingsFromString(TEXT("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"), &autoInfoWindowLogFont);
    propGetFontSettingsFromString(TEXT("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"), &autoTitleWindowLogFont);
    propGetFontSettingsFromString(TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), &autoCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"), &autoMapLabelLogFont);
    propGetFontSettingsFromString(TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), &autoStatisticsLogFont);
    propGetFontSettingsFromString(TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), &autoMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), &autoMapWindowBoldLogFont);
    propGetFontSettingsFromString(TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), &autoTitleSmallWindowLogFont);
  }
}

#ifndef ENABLE_SDL
static void
InitialiseLogfont(LOGFONT* font, const TCHAR* facename, bool variable_pitch,
                  long height, long width, bool bold, bool italic) {
  LOGFONT tmpfont;
  memset((char *)&tmpfont, 0, sizeof(tmpfont));

  _tcscpy(tmpfont.lfFaceName, facename);
  tmpfont.lfPitchAndFamily = (variable_pitch ? VARIABLE_PITCH : FIXED_PITCH)
                             | FF_DONTCARE;
  tmpfont.lfHeight = height;
  tmpfont.lfWidth = width;
  tmpfont.lfWeight = (bold ? FW_BOLD : FW_MEDIUM);
  tmpfont.lfItalic = italic;

  memcpy(font, &tmpfont, sizeof(LOGFONT));
}
#endif

static void
InitialiseFontsAuto()
{
#ifndef ENABLE_SDL
  LOGFONT logfont;
  int FontHeight, FontWidth = 0;

  memset(&autoInfoWindowLogFont, 0, sizeof(LOGFONT));
  memset(&autoTitleWindowLogFont, 0, sizeof(LOGFONT));
  memset(&autoMapWindowLogFont, 0, sizeof(LOGFONT));
  memset(&autoTitleSmallWindowLogFont, 0, sizeof(LOGFONT));
  memset(&autoMapWindowBoldLogFont, 0, sizeof(LOGFONT));
  memset(&autoCDIWindowLogFont, 0, sizeof(LOGFONT));
  memset(&autoMapLabelLogFont, 0, sizeof(LOGFONT));
  memset(&autoStatisticsLogFont, 0, sizeof(LOGFONT));

  if (Layout::square)
    // square
    FontHeight = Layout::Scale(26);
  else
    // portrait & landscape
    FontHeight = Layout::Scale(35);

  // oversize first so can then scale down
  int iFontHeight = (int)(FontHeight * 1.4);

  memset((char *)&logfont, 0, sizeof(logfont));

  if (is_pna())
    /* Only for PNA, since we still do not copy Fonts in their Windows
       memory.  though we could already do it automatically. */
    // VENTA TODO copy DejaVu fonts also for PNA like for PDAs in SD version
    _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  else
    _tcscpy(logfont.lfFaceName, _T("DejaVu Sans Condensed"));

  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
  logfont.lfHeight = iFontHeight;
  logfont.lfWidth = FontWidth;
  logfont.lfWeight = FW_BOLD;
  logfont.lfItalic = false;
  logfont.lfCharSet = ANSI_CHARSET;
  ApplyClearType(&logfont);

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
  memcpy(&autoInfoWindowLogFont, &logfont, sizeof(LOGFONT));

#ifdef WINDOWSPC
  FontHeight = (int)(FontHeight / 1.35);
  FontWidth = (int)(FontWidth / 1.35);
#endif

  InitialiseLogfont(&autoTitleWindowLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * TITLEFONTHEIGHTRATIO),
                    (int)(FontWidth * TITLEFONTWIDTHRATIO), true, false);

  // new font for CDI Scale
  InitialiseLogfont(&autoCDIWindowLogFont, _T("Tahoma"), false,
                    (int)(FontHeight * CDIFONTHEIGHTRATIO),
                    (int)(FontWidth * CDIFONTWIDTHRATIO), false, false);

  // new font for map labels
  InitialiseLogfont(&autoMapLabelLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * MAPFONTHEIGHTRATIO),
                    (int)(FontWidth * MAPFONTWIDTHRATIO), false, true);

  // Font for map other text
  InitialiseLogfont(&autoStatisticsLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * STATISTICSFONTHEIGHTRATIO),
                    (int)(FontWidth * STATISTICSFONTWIDTHRATIO), false, false);

  // new font for map labels
  InitialiseLogfont(&autoMapWindowLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * MAPFONTHEIGHTRATIO * 1.3),
                    (int)(FontWidth * MAPFONTWIDTHRATIO * 1.3), false, false);

  // Font for map bold text
  InitialiseLogfont(&autoMapWindowBoldLogFont, _T("Tahoma"), true,
                    (int)(FontHeight * MAPFONTHEIGHTRATIO * 1.3),
                    0, true, false);

  InitialiseLogfont(&autoTitleSmallWindowLogFont, _T("Tahoma"), true,
                    Layout::Scale(20), Layout::Scale(8), false, false);
#else /* !ENABLE_SDL */
  // XXX implement
#endif /* !ENABLE_SDL */
}

void
InitialiseFonts(const struct Appearance &appearance, RECT rc)
{
  //this routine must be called only at start/restart of XCSoar b/c there are many pointers to these fonts

  InfoWindowFont.reset();
  TitleWindowFont.reset();
  MapWindowFont.reset();
  TitleSmallWindowFont.reset();
  MapWindowBoldFont.reset();
  CDIWindowFont.reset();
  MapLabelFont.reset();
  StatisticsFont.reset();

  InitialiseFontsAuto();

  InitialiseFontsHardCoded(appearance, rc);

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

  InitializeFont(&InfoWindowFont, autoInfoWindowLogFont);
  InitializeFont(&TitleWindowFont, autoTitleWindowLogFont);
  InitializeFont(&CDIWindowFont, autoCDIWindowLogFont);
  InitializeFont(&MapLabelFont, autoMapLabelLogFont);
  InitializeFont(&StatisticsFont, autoStatisticsLogFont);
  InitializeFont(&MapWindowFont, autoMapWindowLogFont);
  InitializeFont(&MapWindowBoldFont, autoMapWindowBoldLogFont);
  InitializeFont(&TitleSmallWindowFont, autoTitleSmallWindowLogFont);
}

void
DeleteFonts()
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
