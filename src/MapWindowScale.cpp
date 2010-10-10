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

#include "MapWindow.hpp"
#include "Appearance.hpp"
#include "Screen/Graphics.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Screen/UnitSymbol.hpp"
#include "Terrain/RasterWeather.hpp"
#include "Task/ProtectedTaskManager.hpp"

#include <math.h>
#include <stdio.h>

fixed
MapWindow::findMapScaleBarSize(const RECT &rc, 
           const MapWindowProjection &projection) const
{
  fixed pixelsize = projection.DistanceScreenToUser(1); // units/pixel
  fixed half_displaysize =
    projection.DistanceScreenToUser((rc.bottom - rc.top) / 2); // units

  // find largest bar size that will fit two of (black and white) in display
  if (half_displaysize > fixed(100))
    return fixed(100) / pixelsize;

  if (half_displaysize > fixed_ten)
    return fixed_ten / pixelsize;

  if (half_displaysize > fixed_one)
    return fixed_one / pixelsize;

  return fixed_one / 10 / pixelsize;
}

void
MapWindow::DrawMapScale2(Canvas &canvas, const RECT &rc, const MapWindowProjection &projection) const
{
  canvas.select(Graphics::hpMapScale);

  bool color = false;
  POINT Start, End = { 0, 0 };

  int barsize = iround(findMapScaleBarSize(rc, projection));

  End.x = rc.right - 1;
  End.y = projection.GetOrigAircraft().y;
  Start = End;
  for (Start.y += barsize; Start.y < rc.bottom + barsize; Start.y += barsize) {
    if (color)
      canvas.white_pen();
    else
      canvas.black_pen();

    canvas.line(Start, End);

    End = Start;
    color = !color;
  }

  color = true;
  End.y = projection.GetOrigAircraft().y;
  Start = End;
  for (Start.y -= barsize; Start.y > rc.top - barsize; Start.y -= barsize) {
    if (color)
      canvas.white_pen();
    else
      canvas.black_pen();

    canvas.line(Start, End);

    End = Start;
    color = !color;
  }
}

static void
draw_bitmap(Canvas &canvas, BitmapCanvas &bitmap_canvas, const Bitmap &bitmap,
            const int x, const int y,
            const unsigned src_x_offset, const unsigned src_y_offset,
            const unsigned src_width, const unsigned src_height)
{
  bitmap_canvas.background_opaque();
  bitmap_canvas.set_text_color(Color::WHITE);
  bitmap_canvas.select(bitmap);
  canvas.scale_copy(x, y, bitmap_canvas, src_x_offset, src_y_offset,
                    src_width, src_height);
}

void
MapWindow::DrawMapScale(Canvas &canvas, const RECT &rc, const MapWindowProjection &projection) const
{
  fixed MapWidth;
  TCHAR ScaleInfo[80];

  int Height;
  Units_t Unit;

  /*
  if (ScaleChangeFeedback)
    MapWidth = projection.RequestDistancePixelsToMeters(rc.right - rc.left);
  else
  */
  MapWidth = projection.DistancePixelsToMeters(rc.right - rc.left);

  canvas.select(Fonts::MapBold);
  Units::FormatUserMapScale(&Unit, MapWidth, ScaleInfo,
                            sizeof(ScaleInfo) / sizeof(TCHAR), false);
  SIZE TextSize = canvas.text_size(ScaleInfo);

  Height = Fonts::MapBold.get_capital_height() + IBLSCALE(2);
  // 2: add 1pix border

  canvas.fill_rectangle(0, rc.bottom - Height,
                        TextSize.cx + IBLSCALE(21), rc.bottom,
                        Color::WHITE);

  canvas.background_transparent();
  canvas.set_text_color(Color::BLACK);

  canvas.text(IBLSCALE(7),
              rc.bottom - Fonts::MapBold.get_ascent_height() - IBLSCALE(1),
              ScaleInfo);

  draw_bitmap(canvas, bitmap_canvas, Graphics::hBmpMapScale,
              0, rc.bottom - Height, 0, 0, 6, 11);
  draw_bitmap(canvas, bitmap_canvas, Graphics::hBmpMapScale,
              IBLSCALE(14) + TextSize.cx, rc.bottom - Height, 6, 0, 8, 11);

  const UnitSymbol *symbol = GetUnitSymbol(Unit);
  if (symbol != NULL)
    symbol->draw(canvas, bitmap_canvas, IBLSCALE(8) + TextSize.cx,
                 rc.bottom - Height);

  int y = rc.bottom - Height - Fonts::Title.get_ascent_height() + IBLSCALE(2);

  ScaleInfo[0] = 0;
  if (SettingsMap().AutoZoom)
    _tcscat(ScaleInfo, _T("AUTO "));

  if (SettingsMap().TargetPan)
    _tcscat(ScaleInfo, _T("TARGET "));
  else if (SettingsMap().EnablePan)
    _tcscat(ScaleInfo, _T("PAN "));

  if (SettingsMap().EnableAuxiliaryInfo)
    _tcscat(ScaleInfo, _T("AUX "));

  if (Basic().gps.Replay)
    _tcscat(ScaleInfo, _T("REPLAY "));

  if (task != NULL && SettingsComputer().BallastTimerActive) {
    TCHAR TEMP[20];
    _stprintf(TEMP, _T("BALLAST %d LITERS"),
              (int)task->get_glide_polar().get_ballast_litres());
    _tcscat(ScaleInfo, TEMP);
  }

  if (weather != NULL && weather->GetParameter() > 0) {
    const TCHAR *label = weather->ItemLabel(weather->GetParameter());
    if (label != NULL)
      _tcscat(ScaleInfo, label);
  }

  if (ScaleInfo[0]) {
    canvas.select(Fonts::Title);
    canvas.background_opaque();
    canvas.set_background_color(Color::WHITE);
    canvas.text(IBLSCALE(1), y, ScaleInfo);
  }
}
