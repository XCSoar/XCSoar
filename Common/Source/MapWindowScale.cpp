/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "MapWindow.h"
#include "Protection.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "InfoBoxLayout.h"
#include <math.h>
#include "Math/FastMath.h"
#include "Screen/Util.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "McReady.h"
#include "InfoBoxManager.h"

#include "RasterTerrain.h"
#include "RasterWeather.h"
#include "Logger.h"
#include "ReplayLogger.hpp"


double MapWindow::findMapScaleBarSize(const RECT rc) {

  int range = rc.bottom-rc.top;
//  int nbars = 0;
//  int nscale = 1;
  double pixelsize = MapScale/GetMapResolutionFactor(); // km/pixel

  // find largest bar size that will fit in display

  double displaysize = range*pixelsize/2; // km

  if (displaysize>100.0) {
    return 100.0/pixelsize;
  }
  if (displaysize>10.0) {
    return 10.0/pixelsize;
  }
  if (displaysize>1.0) {
    return 1.0/pixelsize;
  }
  if (displaysize>0.1) {
    return 0.1/pixelsize;
  }
  // this is as far as is reasonable
  return 0.1/pixelsize;
}


void MapWindow::DrawMapScale2(HDC hDC, const RECT rc,
			      const POINT Orig_Aircraft)
{

  if (Appearance.MapScale2 == apMs2None) return;

  HPEN hpOld   = (HPEN)SelectObject(hDC, MapGfx.hpMapScale);
  HPEN hpWhite = (HPEN)GetStockObject(WHITE_PEN);
  HPEN hpBlack = (HPEN)GetStockObject(BLACK_PEN);

  bool color = false;
  POINT Start, End={0,0};
  bool first=true;

  int barsize = iround(findMapScaleBarSize(rc));

  Start.x = rc.right-1;
  for (Start.y=Orig_Aircraft.y; Start.y<rc.bottom+barsize; Start.y+= barsize) {
    if (color) {
      SelectObject(hDC, hpWhite);
    } else {
      SelectObject(hDC, hpBlack);
    }
    if (!first) {
      ClipLine(hDC, Start, End, rc);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }

  color = true;
  first = true;
  for (Start.y=Orig_Aircraft.y; Start.y>rc.top-barsize; Start.y-= barsize) {
    if (color) {
      SelectObject(hDC, hpWhite);
    } else {
      SelectObject(hDC, hpBlack);
    }
    if (!first) {
      ClipLine(hDC, Start, End, rc);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }

  // draw text as before

  SelectObject(hDC, hpOld);

}


void MapWindow::DrawMapScale(HDC hDC, const RECT rc /* the Map Rect*/,
                             const bool ScaleChangeFeedback)
{


  if (Appearance.MapScale == apMsDefault){

    TCHAR Scale[80];
    TCHAR TEMP[20];
    POINT Start, End;
    HPEN hpOld;
    hpOld = (HPEN)SelectObject(hDC, MapGfx.hpMapScale);

    Start.x = rc.right-IBLSCALE(6); End.x = rc.right-IBLSCALE(6);
    Start.y = rc.bottom-IBLSCALE(30); End.y = Start.y - IBLSCALE(30);
    ClipLine(hDC, Start, End, rc);

    Start.x = rc.right-IBLSCALE(11); End.x = rc.right-IBLSCALE(6);
    End.y = Start.y;
    ClipLine(hDC, Start, End, rc);

    Start.y = Start.y - IBLSCALE(30); End.y = Start.y;
    ClipLine(hDC, Start, End, rc);

    SelectObject(hDC, hpOld);

    if(MapScale <0.1)
    {
      _stprintf(Scale,TEXT("%1.2f"),MapScale);
    }
    else if(MapScale <3)
    {
      _stprintf(Scale,TEXT("%1.1f"),MapScale);
    }
    else
    {
      _stprintf(Scale,TEXT("%1.0f"),MapScale);
    }

    _tcscat(Scale, Units::GetDistanceName());

    if (AutoZoom) {
      _tcscat(Scale,TEXT(" A"));
    }
    if (EnablePan) {
      _tcscat(Scale,TEXT(" PAN"));
    }
    if (EnableAuxiliaryInfo) {
      _tcscat(Scale,TEXT(" AUX"));
    }
    if (ReplayLogger::IsEnabled()) {
      _tcscat(Scale,TEXT(" REPLAY"));
    }
    if (BallastTimerActive) {
      _stprintf(TEMP,TEXT(" BALLAST %3.0f LITERS"), GlidePolar::GetBallastLitres());
      _tcscat(Scale, TEMP);
    }
    TCHAR Buffer[20];
    RASP.ItemLabel(RasterTerrain::render_weather, Buffer);
    if (_tcslen(Buffer)) {
      _tcscat(Scale,TEXT(" "));
      _tcscat(Scale, Buffer);
    }

    SIZE tsize;
    GetTextExtentPoint(hDC, Scale, _tcslen(Scale), &tsize);

    COLORREF whitecolor = RGB(0xd0,0xd0, 0xd0);
    COLORREF blackcolor = RGB(0x20,0x20, 0x20);
    COLORREF origcolor = SetTextColor(hDC, whitecolor);

    SetTextColor(hDC, whitecolor);
    ExtTextOut(hDC, rc.right-IBLSCALE(11)-tsize.cx, End.y+IBLSCALE(8), 0,
               NULL, Scale, _tcslen(Scale), NULL);

    SetTextColor(hDC, blackcolor);
    ExtTextOut(hDC, rc.right-IBLSCALE(10)-tsize.cx, End.y+IBLSCALE(7), 0,
               NULL, Scale, _tcslen(Scale), NULL);

    #ifdef DRAWLOAD
    SelectObject(hDC, MapWindowFont);
    _stprintf(Scale,TEXT("            %d %d ms"), timestats_av,
              misc_tick_count);
    ExtTextOut(hDC, rc.left, rc.top, 0, NULL, Scale, _tcslen(Scale), NULL);
    #endif

    // restore original color
    SetTextColor(hDC, origcolor);

    SelectObject(hDC, hpOld);

  }
  if (Appearance.MapScale == apMsAltA){

    static int LastMapWidth = 0;
    double MapWidth;
    TCHAR ScaleInfo[80];
    TCHAR TEMP[20];

    HFONT          oldFont;
    int            Height;
    SIZE           TextSize;
    HBRUSH         oldBrush;
    HPEN           oldPen;
    COLORREF       oldTextColor;
    HBITMAP        oldBitMap;
    Units_t        Unit;

    if (ScaleChangeFeedback)
      MapWidth = (RequestMapScale * rc.right)/DISTANCEMODIFY/GetMapResolutionFactor();
    else
      MapWidth = (MapScale * rc.right)/DISTANCEMODIFY/GetMapResolutionFactor();

    oldFont = (HFONT)SelectObject(hDC, MapWindowBoldFont);
    Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo,
                              sizeof(ScaleInfo)/sizeof(TCHAR));
    GetTextExtentPoint(hDC, ScaleInfo, _tcslen(ScaleInfo), &TextSize);
    LastMapWidth = (int)MapWidth;

    Height = Appearance.MapWindowBoldFont.CapitalHeight+IBLSCALE(2);
    // 2: add 1pix border

    oldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));
    oldPen = (HPEN)SelectObject(hDC, GetStockObject(WHITE_PEN));
    Rectangle(hDC, 0, rc.bottom-Height,
              TextSize.cx + IBLSCALE(21), rc.bottom);
    if (ScaleChangeFeedback){
      SetBkMode(hDC, TRANSPARENT);
      oldTextColor = SetTextColor(hDC, RGB(0xff,0,0));
    }else
      oldTextColor = SetTextColor(hDC, RGB(0,0,0));

    ExtTextOut(hDC, IBLSCALE(7),
               rc.bottom-Appearance.MapWindowBoldFont.AscentHeight-IBLSCALE(1),
               0, NULL, ScaleInfo, _tcslen(ScaleInfo), NULL);

    oldBitMap = (HBITMAP)SelectObject(hDCTemp, MapGfx.hBmpMapScale);

    DrawBitmapX(hDC, 0, rc.bottom-Height, 6, 11, hDCTemp, 0, 0, SRCCOPY);
    DrawBitmapX(hDC,
           IBLSCALE(14)+TextSize.cx,
           rc.bottom-Height, 8, 11, hDCTemp, 6, 0, SRCCOPY);

    if (!ScaleChangeFeedback){
      HBITMAP Bmp;
      POINT   BmpPos, BmpSize;

      if (Units::GetUnitBitmap(Unit, &Bmp, &BmpPos, &BmpSize, 0)){
        HBITMAP oldBitMapa = (HBITMAP)SelectObject(hDCTemp, Bmp);

        DrawBitmapX(hDC,
                    IBLSCALE(8)+TextSize.cx, rc.bottom-Height,
                    BmpSize.x, BmpSize.y,
                    hDCTemp, BmpPos.x, BmpPos.y, SRCCOPY);
        SelectObject(hDCTemp, oldBitMapa);
      }
    }

    int y = rc.bottom-Height-
      (Appearance.TitleWindowFont.AscentHeight+IBLSCALE(2));
    if (!ScaleChangeFeedback){
      // bool FontSelected = false;
      // TODO code: gettext these
      ScaleInfo[0] = 0;
      if (AutoZoom) {
        _tcscat(ScaleInfo, TEXT("AUTO "));
      }
      if (TargetPan) {
        _tcscat(ScaleInfo, TEXT("TARGET "));
      } else if (EnablePan) {
        _tcscat(ScaleInfo, TEXT("PAN "));
      }
      if (EnableAuxiliaryInfo) {
        _tcscat(ScaleInfo, TEXT("AUX "));
      }
      if (ReplayLogger::IsEnabled()) {
        _tcscat(ScaleInfo, TEXT("REPLAY "));
      }
      if (BallastTimerActive) {
        _stprintf(TEMP,TEXT("BALLAST %3.0f LITERS"), GlidePolar::GetBallastLitres());
        _tcscat(ScaleInfo, TEMP);
      }
      TCHAR Buffer[20];
      RASP.ItemLabel(RasterTerrain::render_weather, Buffer);
      if (_tcslen(Buffer)) {
        _tcscat(ScaleInfo, Buffer);
      }

      if (ScaleInfo[0]) {
        SelectObject(hDC, TitleWindowFont);
        // FontSelected = true;
        ExtTextOut(hDC, IBLSCALE(1), y, 0, NULL, ScaleInfo,
                   _tcslen(ScaleInfo), NULL);
        y -= (Appearance.TitleWindowFont.CapitalHeight+IBLSCALE(1));
      }
    }

    #ifdef DRAWLOAD
    SelectObject(hDC, MapWindowFont);
    _stprintf(ScaleInfo,TEXT("    %d %d ms"),
              timestats_av,
              misc_tick_count);

    ExtTextOut(hDC, rc.left, rc.top, 0, NULL, ScaleInfo,
               _tcslen(ScaleInfo), NULL);
    #endif

    SetTextColor(hDC, oldTextColor);
    SelectObject(hDC, oldPen);
    SelectObject(hDC, oldFont);
    SelectObject(hDC, oldBrush);
    SelectObject(hDCTemp, oldBitMap);

  }

}
