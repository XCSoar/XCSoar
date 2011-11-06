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

#include "WindArrowRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"
#include "Math/Screen.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"
#include "SettingsMap.hpp"

#include <tchar.h>
#include <cstdio>

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const SpeedVector wind, const RasterPoint pos,
                        const PixelRect rc, bool with_tail)
{
  canvas.select(Fonts::MapBold);
  UPixelScalar text_width = canvas.text_size(_T("99")).cx / 2;

  canvas.select(look.hpWind);
  canvas.select(look.hbWind);

  PixelScalar wmag = iround(4 * wind.norm);

  PixelScalar kx = text_width / Layout::FastScale(1) / 2;

  RasterPoint arrow[7] = {
      { 0, -20 },
      { -6, -26 },
      { 0, -20 },
      { 6, -26 },
      { 0, -20 },
      { (PixelScalar)(8 + kx), -24 },
      { (PixelScalar)(-8 - kx), -24 }
  };

  for (int i = 1; i < 4; i++)
    arrow[i].y -= wmag;

  PolygonRotateShift(arrow, 7, pos.x, pos.y, wind.bearing - screen_angle);

  canvas.polygon(arrow, 5);

  if (with_tail) {
    RasterPoint tail[2] = {
      { 0, Layout::FastScale(-20) },
      { 0, Layout::FastScale(-26 - min(PixelScalar(20), wmag) * 3) },
    };

    PolygonRotateShift(tail, 2, pos.x, pos.y, wind.bearing - screen_angle);

    // optionally draw dashed line
    canvas.select(look.hpWindTail);
    canvas.line(tail[0], tail[1]);
  }

  TCHAR sTmp[12];
  _stprintf(sTmp, _T("%i"), iround(Units::ToUserWindSpeed(wind.norm)));

  canvas.set_text_color(COLOR_BLACK);

  TextInBoxMode style;
  style.align = A_CENTER;
  style.mode = RM_OUTLINED;

  if (arrow[5].y >= arrow[6].y)
    TextInBox(canvas, sTmp, arrow[5].x - kx, arrow[5].y, style, rc);
  else
    TextInBox(canvas, sTmp, arrow[6].x - kx, arrow[6].y, style, rc);
}

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const RasterPoint pos, const PixelRect rc,
                        const DerivedInfo &calculated,
                        const SETTINGS_MAP &settings)
{
  if (!calculated.wind_available)
    return;

  // don't bother drawing it if not significant
  if (calculated.wind.norm < fixed_one)
    return;

  WindArrowRenderer::Draw(canvas, screen_angle, calculated.wind, pos, rc,
                          settings.wind_arrow_style == 1);
}
