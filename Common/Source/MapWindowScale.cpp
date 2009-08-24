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
#include "McReady.h"
#include "InfoBoxManager.h"

#include "RasterTerrain.h"
#include "RasterWeather.h"
#include "Logger.h"
#include "SettingsUser.hpp"

extern HFONT  MapWindowBoldFont;
extern HFONT  TitleWindowFont;

double MapWindow::LimitMapScale(double value) {

  double minreasonable;

  minreasonable = 0.05;

  if (AutoZoom && DisplayMode != dmCircling) {
    if (AATEnabled && (ActiveWayPoint>0)) {
      minreasonable = 0.88;
    } else {
      minreasonable = 0.44;
    }
  }

  if (ScaleListCount>0) {
    return FindMapScale(max(minreasonable,min(160.0,value)));
  } else {
    return max(minreasonable,min(160.0,value));
  }
}


void MapWindow::Event_SetZoom(double value) {

  static double lastRequestMapScale = RequestMapScale;

  RequestMapScale = LimitMapScale(value);
  if (lastRequestMapScale != RequestMapScale){
    lastRequestMapScale = RequestMapScale;
    BigZoom = true;
    RefreshMap();
  }
}


void MapWindow::Event_ScaleZoom(int vswitch) {

  static double lastRequestMapScale = RequestMapScale;
  double value = RequestMapScale;
  static int nslow=0;

  // For best results, zooms should be multiples or roots of 2

  if (ScaleListCount > 0){
    value = FindMapScale(RequestMapScale);
    value = StepMapScale(-vswitch);
  } else {

    if (abs(vswitch)>=4) {
      nslow++;
      if (nslow %2 != 0) {
        // JMW disabled        return;
      }
      if (vswitch==4) {
        vswitch = 1;
      }
      if (vswitch==-4) {
        vswitch = -1;
      }
    }
    if (vswitch==1) { // zoom in a little
      value /= 1.414;
    }
    if (vswitch== -1) { // zoom out a little
      value *= 1.414;
    }
    if (vswitch==2) { // zoom in a lot
      value /= 2.0;
    }
    if (vswitch== -2) { // zoom out a lot
      value *= 2.0;
    }

  }
  RequestMapScale = LimitMapScale(value);

  if (lastRequestMapScale != RequestMapScale){
    lastRequestMapScale = RequestMapScale;
    BigZoom = true;
    RefreshMap();

    //    DrawMapScale(hdcScreen, MapRect, true);
    // JMW this is bad, happening from wrong thread.
  }
}


int MapWindow::GetMapResolutionFactor(void) {
  return IBLSCALE(30);
}

double MapWindow::StepMapScale(int Step){
  static int nslow=0;
  if (abs(Step)>=4) {
    nslow++;
    //    if (nslow %2 == 0) {
    ScaleCurrent += Step/4;
    //    }
  } else {
    ScaleCurrent += Step;
  }
  ScaleCurrent = max(0,min(ScaleListCount-1, ScaleCurrent));
  return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
         /(IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right)));
}

double MapWindow::FindMapScale(double Value){

  int    i;
  double BestFit = 99999;
  int    BestFitIdx=-1;
  double DesiredScale =
    (Value*IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right))/GetMapResolutionFactor();

  for (i=0; i<ScaleListCount; i++){
    double err = fabs(DesiredScale - ScaleList[i])/DesiredScale;
    if (err < BestFit){
      BestFit = err;
      BestFitIdx = i;
    }
  }

  if (BestFitIdx != -1){
    ScaleCurrent = BestFitIdx;
    return((ScaleList[ScaleCurrent]*GetMapResolutionFactor())
           /IBLSCALE(/*Appearance.DefaultMapWidth*/ MapRect.right));
  }
  return(Value);
}



void MapWindow::ModifyMapScale(void) {
  // limit zoomed in so doesn't reach silly levels
  RequestMapScale = LimitMapScale(RequestMapScale); // FIX VENTA remove limit
  MapScaleOverDistanceModify = RequestMapScale/DISTANCEMODIFY;
  ResMapScaleOverDistanceModify =
    GetMapResolutionFactor()/MapScaleOverDistanceModify;
  DrawScale = MapScaleOverDistanceModify;
  DrawScale = DrawScale/111194;
  DrawScale = GetMapResolutionFactor()/DrawScale;
  InvDrawScale = 1.0/DrawScale;
  MapScale = RequestMapScale;
}


void MapWindow::UpdateMapScale()
{
  static int AutoMapScaleWaypointIndex = -1;
  static double StartingAutoMapScale=0.0;
  double AutoZoomFactor;

  bool useraskedforchange = false;

  // if there is user intervention in the scale
  if(MapScale != RequestMapScale)
    {
      ModifyMapScale();
      useraskedforchange = true;
    }

  double wpd;
  if (TargetPan) {
    wpd = TargetZoomDistance;
  } else {
    wpd = DerivedDrawInfo.ZoomDistance;
  }
  if (TargetPan) {
    // set scale exactly so that waypoint distance is the zoom factor
    // across the screen
    RequestMapScale = LimitMapScale(wpd
                                    *DISTANCEMODIFY/ 4.0);
    ModifyMapScale();
    return;
  }

  if (AutoZoom) {
    if(wpd > 0)
      {

	if(
	   (((DisplayOrientation == NORTHTRACK)
	     &&(DisplayMode != dmCircling))
	    ||(DisplayOrientation == NORTHUP)
	    ||
	    (((DisplayOrientation == NORTHCIRCLE)
	      || (DisplayOrientation == TRACKCIRCLE))
	     && (DisplayMode == dmCircling) ))
	   && !TargetPan
	   )
	  {
	    AutoZoomFactor = 2.5;
	  }
	else
	  {
	    AutoZoomFactor = 4;
	  }

	if(
	   (wpd < ( AutoZoomFactor * MapScaleOverDistanceModify))
	   ||
	   (StartingAutoMapScale==0.0))
	  {
	    // waypoint is too close, so zoom in
	    // OR just turned waypoint

	    // this is the first time this waypoint has gotten close,
	    // so save original map scale

	    if (StartingAutoMapScale==0.0) {
	      StartingAutoMapScale = MapScale;
	    }

	    // set scale exactly so that waypoint distance is the zoom factor
	    // across the screen
	    RequestMapScale = LimitMapScale(wpd
					    *DISTANCEMODIFY/ AutoZoomFactor);
	    ModifyMapScale();

	  } else {

	  if (useraskedforchange) {

	    // user asked for a zoom change and it was achieved, so
	    // reset starting map scale


	    ////?TODO enhancement: for frank          StartingAutoMapScale = MapScale;
	  }

	}
      }
  } else {

    // reset starting map scale for auto zoom if momentarily switch
    // off autozoom
    //    StartingAutoMapScale = RequestMapScale;
  }

  if (TargetPan) {
    return;
  }

  LockTaskData();  // protect from external task changes
#ifdef HAVEEXCEPTIONS
  __try{
#endif
    // if we aren't looking at a waypoint, see if we are now
    if (AutoMapScaleWaypointIndex == -1) {
      if (ValidTaskPoint(ActiveWayPoint)) {
	AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;
      }
    }

    // if there is an active waypoint
    if (ValidTaskPoint(ActiveWayPoint)) {

      // if the current zoom focused waypoint has changed...
      if (AutoMapScaleWaypointIndex != Task[ActiveWayPoint].Index) {
	AutoMapScaleWaypointIndex = Task[ActiveWayPoint].Index;

	// zoom back out to where we were before
	if (StartingAutoMapScale> 0.0) {
	  RequestMapScale = StartingAutoMapScale;
	}

	// reset search for new starting zoom level
	StartingAutoMapScale = 0.0;
      }

    }
#ifdef HAVEEXCEPTIONS
  }__finally
#endif
     {
       UnlockTaskData();
     }

}


double MapWindow::GetApproxScreenRange() {
  return (MapScale * max(MapRectBig.right-MapRectBig.left,
                         MapRectBig.bottom-MapRectBig.top))
    *1000.0/GetMapResolutionFactor();
}



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

  HPEN hpOld   = (HPEN)SelectObject(hDC, hpMapScale);
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
    hpOld = (HPEN)SelectObject(hDC, hpMapScale);

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
      _stprintf(TEMP,TEXT(" BALLAST %3.0f LITERS"), WEIGHTS[2]*BALLAST);
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

    oldBitMap = (HBITMAP)SelectObject(hDCTemp, hBmpMapScale);

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
        _stprintf(TEMP,TEXT("BALLAST %3.0f LITERS"), WEIGHTS[2]*BALLAST);
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
