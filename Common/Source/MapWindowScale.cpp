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


void MapWindow::DrawMapScale2(Canvas &canvas, const RECT rc,
			      const POINT Orig_Aircraft)
{

  if (Appearance.MapScale2 == apMs2None) return;

  canvas.select(MapGfx.hpMapScale);

  bool color = false;
  POINT Start, End={0,0};
  bool first=true;

  int barsize = iround(findMapScaleBarSize(rc));

  Start.x = rc.right-1;
  for (Start.y=Orig_Aircraft.y; Start.y<rc.bottom+barsize; Start.y+= barsize) {
    if (color) {
      canvas.white_pen();
    } else {
      canvas.black_pen();
    }
    if (!first) {
      canvas.clipped_line(Start, End, rc);
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
      canvas.white_pen();
    } else {
      canvas.black_pen();
    }
    if (!first) {
      canvas.clipped_line(Start, End, rc);
    } else {
      first=false;
    }
    End = Start;
    color = !color;
  }

  // draw text as before
}


void MapWindow::DrawMapScale(Canvas &canvas, const RECT rc /* the Map Rect*/,
                             const bool ScaleChangeFeedback)
{


  if (Appearance.MapScale == apMsDefault){

    TCHAR Scale[80];
    TCHAR TEMP[20];
    POINT Start, End;
    canvas.select(MapGfx.hpMapScale);

    Start.x = rc.right-IBLSCALE(6); End.x = rc.right-IBLSCALE(6);
    Start.y = rc.bottom-IBLSCALE(30); End.y = Start.y - IBLSCALE(30);
    canvas.clipped_line(Start, End, rc);

    Start.x = rc.right-IBLSCALE(11); End.x = rc.right-IBLSCALE(6);
    End.y = Start.y;
    canvas.clipped_line(Start, End, rc);

    Start.y = Start.y - IBLSCALE(30); End.y = Start.y;
    canvas.clipped_line(Start, End, rc);

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

    SIZE tsize = canvas.text_size(Scale);

    canvas.set_text_color(Color(0xd0, 0xd0, 0xd0));
    canvas.text(rc.right - IBLSCALE(11) - tsize.cx, End.y + IBLSCALE(8),
                Scale);

    canvas.set_text_color(Color(0x20, 0x20, 0x20));
    canvas.text(rc.right - IBLSCALE(10) - tsize.cx, End.y + IBLSCALE(7),
                Scale);

    #ifdef DRAWLOAD
    canvas.select(MapWindowFont);
    _stprintf(Scale,TEXT("            %d %d ms"), timestats_av,
              0);
    canvas.text(rc.left, rc.top, Scale);
    #endif
  }
  if (Appearance.MapScale == apMsAltA){

    static int LastMapWidth = 0;
    double MapWidth;
    TCHAR ScaleInfo[80];
    TCHAR TEMP[20];

    int            Height;
    Units_t        Unit;

    if (ScaleChangeFeedback)
      MapWidth = (RequestMapScale * rc.right)/DISTANCEMODIFY/GetMapResolutionFactor();
    else
      MapWidth = (MapScale * rc.right)/DISTANCEMODIFY/GetMapResolutionFactor();

    canvas.select(MapWindowBoldFont);
    Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo,
                              sizeof(ScaleInfo)/sizeof(TCHAR));
    SIZE TextSize = canvas.text_size(ScaleInfo);
    LastMapWidth = (int)MapWidth;

    Height = Appearance.MapWindowBoldFont.CapitalHeight+IBLSCALE(2);
    // 2: add 1pix border

    canvas.white_brush();
    canvas.white_pen();
    canvas.rectangle(0, rc.bottom - Height,
                     TextSize.cx + IBLSCALE(21), rc.bottom);
    if (ScaleChangeFeedback){
      canvas.background_transparent();
      canvas.set_text_color(Color(0xff, 0, 0));
    }else
      canvas.set_text_color(Color(0, 0, 0));

    canvas.text(IBLSCALE(7),
                rc.bottom - Appearance.MapWindowBoldFont.AscentHeight - IBLSCALE(1),
                ScaleInfo);

    draw_bitmap(canvas, MapGfx.hBmpMapScale, 
		0, rc.bottom-Height, 
		0, 0, 6, 11, false);
    draw_bitmap(canvas, MapGfx.hBmpMapScale, 
		IBLSCALE(14)+TextSize.cx, rc.bottom-Height,
		6, 0, 8, 11, false);

    if (!ScaleChangeFeedback){
      const Bitmap *Bmp;
      POINT   BmpPos, BmpSize;

      if (Units::GetUnitBitmap(Unit, &Bmp, &BmpPos, &BmpSize, 0)){
	draw_bitmap(canvas, *Bmp, IBLSCALE(8) + TextSize.cx, rc.bottom - Height,
		    BmpPos.x, BmpPos.y, BmpSize.x, BmpSize.y, false);
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
        canvas.select(TitleWindowFont);
        // FontSelected = true;
        canvas.text(IBLSCALE(1), y, ScaleInfo);
        y -= (Appearance.TitleWindowFont.CapitalHeight+IBLSCALE(1));
      }
    }

    #ifdef DRAWLOAD
    canvas.select(MapWindowFont);
    _stprintf(ScaleInfo,TEXT("    %d %d ms"),
              timestats_av,
              misc_tick_count);

    canvas.text(rc.left, rc.top, ScaleInfo);
    #endif

  }

}
