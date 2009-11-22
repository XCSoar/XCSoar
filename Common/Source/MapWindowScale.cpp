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

#include "MapWindow.h"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "SettingsTask.hpp"
#include "Appearance.hpp"
#include "InfoBoxLayout.h"
#include "Math/FastMath.h"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/UnitSymbol.hpp"
#include "McReady.h"
#include "Components.hpp"
#include "options.h" /* for IBLSCALE() */
#include "RasterTerrain.h"
#include "RasterWeather.h"

#include <math.h>

double MapWindow::findMapScaleBarSize(const RECT rc) {
  double pixelsize = DistanceScreenToUser(1); // units/pixel
  double half_displaysize = DistanceScreenToUser((rc.bottom-rc.top)/2); // units

  // find largest bar size that will fit two of (black and white) in display

  if (half_displaysize>100.0) {
    return 100.0/pixelsize;
  }
  if (half_displaysize>10.0) {
    return 10.0/pixelsize;
  }
  if (half_displaysize>1.0) {
    return 1.0/pixelsize;
  }
  if (half_displaysize>0.1) {
    return 0.1/pixelsize;
  }
  // this is as far as is reasonable
  return 0.1/pixelsize;
}


void MapWindow::DrawMapScale2(Canvas &canvas, const RECT rc)
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
      canvas.line(Start, End);
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
      canvas.line(Start, End);
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
  if (Appearance.MapScale == apMsAltA) {

    static int LastMapWidth = 0;
    double MapWidth;
    TCHAR ScaleInfo[80];
    TCHAR TEMP[20];

    int            Height;
    Units_t        Unit;

    if (ScaleChangeFeedback)
      MapWidth = RequestDistancePixelsToMeters(rc.right-rc.left);
    else
      MapWidth = DistancePixelsToMeters(rc.right-rc.left);

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
      const UnitSymbol *symbol = GetUnitSymbol(Unit);

      if (symbol != NULL) {
        POINT origin = symbol->get_origin(UnitSymbol::NORMAL);
        SIZE size = symbol->get_size();

        draw_bitmap(canvas, *symbol,
                    IBLSCALE(8) + TextSize.cx, rc.bottom - Height,
                    origin.x, origin.y, size.cx, size.cy, false);
      }
    }

    int y = rc.bottom-Height-
      (Appearance.TitleWindowFont.AscentHeight+IBLSCALE(2));
    if (!ScaleChangeFeedback){
      // bool FontSelected = false;
      // TODO code: gettext these
      ScaleInfo[0] = 0;
      if (SettingsMap().AutoZoom) {
        _tcscat(ScaleInfo, TEXT("AUTO "));
      }
      if (SettingsMap().TargetPan) {
        _tcscat(ScaleInfo, TEXT("TARGET "));
      } else if (SettingsMap().EnablePan) {
        _tcscat(ScaleInfo, TEXT("PAN "));
      }
      if (SettingsMap().EnableAuxiliaryInfo) {
        _tcscat(ScaleInfo, TEXT("AUX "));
      }
      if (Basic().Replay) {
        _tcscat(ScaleInfo, TEXT("REPLAY "));
      }
      if (SettingsComputer().BallastTimerActive) {
        _stprintf(TEMP,TEXT("BALLAST %3.0f LITERS"),
		  GlidePolar::GetBallastLitres());
        _tcscat(ScaleInfo, TEMP);
      }
      TCHAR Buffer[20];
      RASP.ItemLabel(RASP.GetParameter(), Buffer);
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
    _stprintf(ScaleInfo,TEXT("    %d ms"),
              GetAverageTime());

    canvas.text(rc.left, rc.top, ScaleInfo);
    #endif

  }

}
