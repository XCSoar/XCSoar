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

bool UseCustomFonts;

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
InitializeOneFont(Font *theFont, LOGFONT autoLogFont,
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
InitialiseFontsHardCoded(const struct Appearance &appearance, RECT rc,
                         LOGFONT *ptrhardInfoWindowLogFont,
                         LOGFONT *ptrhardTitleWindowLogFont,
                         LOGFONT *ptrhardMapWindowLogFont,
                         LOGFONT *ptrhardTitleSmallWindowLogFont,
                         LOGFONT *ptrhardMapWindowBoldLogFont,
                         LOGFONT *ptrhardCDIWindowLogFont, // New
                         LOGFONT *ptrhardMapLabelLogFont,
                         LOGFONT *ptrhardStatisticsLogFont)
{

  int ScreenSize = 0;

  int iWidth = rc.right - rc.left;
  int iHeight = rc.bottom - rc.top;

  if (iWidth == 240 && iHeight == 320)
    ScreenSize = (ScreenSize_t)ss240x320; // QVGA	portrait
  else if (iWidth == 480 && iHeight == 640)
    ScreenSize = (ScreenSize_t)ss480x640; //  VGA
  else if (iWidth == 480 && iHeight == 800)
    ScreenSize = (ScreenSize_t)ss480x800;
  else if (iWidth == 480 && iHeight == 272)
    ScreenSize = (ScreenSize_t)ss480x272; // WQVGA	landscape
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

  if (is_pna()) {
    if (ScreenSize == (ScreenSize_t)ss480x272) {
      // WQVGA  e.g. MIO
      propGetFontSettingsFromString(TEXT("28,0,0,0,800,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
      propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardCDIWindowLogFont);
      propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont); // RLD 16 works well too
      propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);//  (RLD is this used?)
      propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
      if (appearance.InfoBoxGeom == 5)
        // We don't use vario gauge in landscape geo5 anymore.. but doesn't hurt.
        SetGlobalEllipse(1.32f);
      else
        SetGlobalEllipse(1.1f);
    } else if (ScreenSize == (ScreenSize_t)ss480x234) {
      // e.g. Messada 2440
      propGetFontSettingsFromString(TEXT("22,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
      propGetFontSettingsFromString(TEXT("18,0,0,0,500,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
      propGetFontSettingsFromString(TEXT("20,0,0,0,400,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
      propGetFontSettingsFromString(TEXT("28,0,0,0,400,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardCDIWindowLogFont);
      propGetFontSettingsFromString(TEXT("14,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont); // RLD 16 works well too
      propGetFontSettingsFromString(TEXT("20,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);//  (RLD is this used?)
      propGetFontSettingsFromString(TEXT("18,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
      propGetFontSettingsFromString(TEXT("16,0,0,0,500,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
      SetGlobalEllipse(1.1f); // to be checked, TODO
    } else if (ScreenSize == (ScreenSize_t)ss800x480) {// e.g. ipaq 31x {
      switch (appearance.InfoBoxGeom) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 6: // standard landscape
        propGetFontSettingsFromString(TEXT("56,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("20,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
        SetGlobalEllipse(1.1f);
        break;
      case 4:
      case 5:
        propGetFontSettingsFromString(TEXT("64,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("26,0,0,0,600,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
        SetGlobalEllipse(1.32f);
        break;
      case 7:
        propGetFontSettingsFromString(TEXT("66,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("23,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
        break;

        // This is a failsafe with an impossible setting so that you know
        // something is going very wrong.
      default:
        propGetFontSettingsFromString(TEXT("30,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardInfoWindowLogFont);
        propGetFontSettingsFromString(TEXT("10,0,0,0,200,0,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleWindowLogFont);
        break;
      } // special geometry cases for 31x

      propGetFontSettingsFromString(TEXT("16,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardTitleSmallWindowLogFont);
      propGetFontSettingsFromString(TEXT("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardCDIWindowLogFont);
      propGetFontSettingsFromString(TEXT("28,0,0,0,100,1,0,0,0,0,0,3,2,Tahoma"), ptrhardMapLabelLogFont);
      propGetFontSettingsFromString(TEXT("48,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardStatisticsLogFont);
      propGetFontSettingsFromString(TEXT("36,0,0,0,400,0,0,0,0,0,0,3,2,Tahoma"), ptrhardMapWindowLogFont);
      propGetFontSettingsFromString(TEXT("32,0,0,0,600,0,0,0,0,0,0,3,2,TahomaBD"), ptrhardMapWindowBoldLogFont);
    }
  }

  if (is_altair()) {
    // RLD Altair also loads these in registry and by default, uses the registry
    propGetFontSettingsFromString(TEXT("24,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicTwentyFourCond"), ptrhardInfoWindowLogFont);
    propGetFontSettingsFromString(TEXT("10,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicNineCond"), ptrhardTitleWindowLogFont);
    propGetFontSettingsFromString(TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), ptrhardCDIWindowLogFont);
    propGetFontSettingsFromString(TEXT("13,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicTwelveCond"), ptrhardMapLabelLogFont);
    propGetFontSettingsFromString(TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), ptrhardStatisticsLogFont);
    propGetFontSettingsFromString(TEXT("15,0,0,0,500,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), ptrhardMapWindowLogFont);
    propGetFontSettingsFromString(TEXT("15,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicFourteenCond"), ptrhardMapWindowBoldLogFont);
    propGetFontSettingsFromString(TEXT("19,0,0,0,700,0,0,0,0,0,0,3,2,RasterGothicEighteenCond"), ptrhardTitleSmallWindowLogFont);
  }
}

static void
InitialiseFontsAuto(LOGFONT *ptrautoInfoWindowLogFont,
                    LOGFONT *ptrautoTitleWindowLogFont,
                    LOGFONT *ptrautoMapWindowLogFont,
                    LOGFONT *ptrautoTitleSmallWindowLogFont,
                    LOGFONT *ptrautoMapWindowBoldLogFont,
                    LOGFONT *ptrautoCDIWindowLogFont, // New
                    LOGFONT *ptrautoMapLabelLogFont,
                    LOGFONT *ptrautoStatisticsLogFont)
{
#ifndef ENABLE_SDL
  LOGFONT logfont;
  int FontHeight, FontWidth;

  memset((char *)ptrautoInfoWindowLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoTitleWindowLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoMapWindowLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoTitleSmallWindowLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoMapWindowBoldLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoCDIWindowLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoMapLabelLogFont, 0, sizeof(LOGFONT));
  memset((char *)ptrautoStatisticsLogFont, 0, sizeof(LOGFONT));

  if (Layout::square)
    // square
    FontHeight = Layout::Scale(26);
  else
    // portrait & landscape
    FontHeight = Layout::Scale(35);

  // oversize first so can then scale down
  int iFontHeight = (int)(FontHeight * 1.4);

  // JMW this should be done so closest font is found
  FontWidth = 0;

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
  memcpy((void *)ptrautoInfoWindowLogFont, &logfont, sizeof(LOGFONT));

#ifdef WINDOWSPC
  FontHeight = (int)(FontHeight / 1.35);
  FontWidth = (int)(FontWidth / 1.35);
#endif

  memset((char *)&logfont, 0, sizeof(logfont));
  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
  logfont.lfHeight = (int)(FontHeight * TITLEFONTHEIGHTRATIO);
  logfont.lfWidth = (int)(FontWidth * TITLEFONTWIDTHRATIO);
  logfont.lfWeight = FW_BOLD;
  memcpy((void *)ptrautoTitleWindowLogFont, &logfont, sizeof(LOGFONT));

  // new font for CDI Scale
  memset((char *)&logfont, 0, sizeof(logfont));
  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
  logfont.lfHeight = (int)(FontHeight * CDIFONTHEIGHTRATIO);
  logfont.lfWidth = (int)(FontWidth * CDIFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  memcpy((void *)ptrautoCDIWindowLogFont, &logfont, sizeof(LOGFONT));

  // new font for map labels
  memset((char *)&logfont, 0, sizeof(logfont));
  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
  logfont.lfHeight = (int)(FontHeight * MAPFONTHEIGHTRATIO);
  logfont.lfWidth = (int)(FontWidth * MAPFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  logfont.lfItalic = TRUE;
  memcpy((void *)ptrautoMapLabelLogFont, &logfont, sizeof(LOGFONT));

  // Font for map other text
  memset((char *)&logfont, 0, sizeof(logfont));
  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
  logfont.lfHeight = (int)(FontHeight * STATISTICSFONTHEIGHTRATIO);
  logfont.lfWidth = (int)(FontWidth * STATISTICSFONTWIDTHRATIO);
  logfont.lfWeight = FW_MEDIUM;
  memcpy((void *)ptrautoStatisticsLogFont, &logfont, sizeof(LOGFONT));

  // new font for map labels
  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
  logfont.lfHeight = (int)(FontHeight * MAPFONTHEIGHTRATIO * 1.3);
  logfont.lfWidth = (int)(FontWidth * MAPFONTWIDTHRATIO * 1.3);
  logfont.lfWeight = FW_MEDIUM;
  memcpy((void *)ptrautoMapWindowLogFont, &logfont, sizeof(LOGFONT));

  // Font for map bold text
  _tcscpy(logfont.lfFaceName, _T("Tahoma"));
  logfont.lfWeight = FW_BOLD;
  logfont.lfWidth = 0; // JMW (int)(FontWidth*MAPFONTWIDTHRATIO*1.3) +2;
  memcpy((void *)ptrautoMapWindowBoldLogFont, &logfont, sizeof(LOGFONT));

  // TODO code: create font settings for this one...
  memset((char *)&logfont, 0, sizeof(logfont));
  _tcscpy(logfont.lfFaceName, _T(GLOBALFONT));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
  logfont.lfHeight = IBLSCALE(20);
  logfont.lfWidth = IBLSCALE(8);
  logfont.lfWeight = FW_MEDIUM;
  memcpy((void *)ptrautoTitleSmallWindowLogFont, &logfont, sizeof(LOGFONT));
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

  InitialiseFontsAuto(&autoInfoWindowLogFont,
                      &autoTitleWindowLogFont,
                      &autoMapWindowLogFont,
                      &autoTitleSmallWindowLogFont,
                      &autoMapWindowBoldLogFont,
                      &autoCDIWindowLogFont,
                      &autoMapLabelLogFont,
                      &autoStatisticsLogFont);

  InitialiseFontsHardCoded(appearance, rc,
                           &autoInfoWindowLogFont,
                           &autoTitleWindowLogFont,
                           &autoMapWindowLogFont,
                           &autoTitleSmallWindowLogFont,
                           &autoMapWindowBoldLogFont,
                           &autoCDIWindowLogFont,
                           &autoMapLabelLogFont,
                           &autoStatisticsLogFont);

  if (UseCustomFonts) {
    LoadCustomFont(&InfoWindowFont, szProfileFontInfoWindowFont);
    LoadCustomFont(&TitleWindowFont, szProfileFontTitleWindowFont);
    LoadCustomFont(&CDIWindowFont, szProfileFontCDIWindowFont);
    LoadCustomFont(&MapLabelFont, szProfileFontMapLabelFont);
    LoadCustomFont(&StatisticsFont, szProfileFontStatisticsFont);
    LoadCustomFont(&MapWindowFont, szProfileFontMapWindowFont);
    LoadCustomFont(&MapWindowBoldFont, szProfileFontMapWindowBoldFont);
    LoadCustomFont(&TitleSmallWindowFont, szProfileFontTitleSmallWindowFont);
  }

  InitializeOneFont(&InfoWindowFont, autoInfoWindowLogFont);
  InitializeOneFont(&TitleWindowFont, autoTitleWindowLogFont);
  InitializeOneFont(&CDIWindowFont, autoCDIWindowLogFont);
  InitializeOneFont(&MapLabelFont, autoMapLabelLogFont);
  InitializeOneFont(&StatisticsFont, autoStatisticsLogFont);
  InitializeOneFont(&MapWindowFont, autoMapWindowLogFont);
  InitializeOneFont(&MapWindowBoldFont, autoMapWindowBoldLogFont);
  InitializeOneFont(&TitleSmallWindowFont, autoTitleSmallWindowLogFont);
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
