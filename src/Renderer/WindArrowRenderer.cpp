/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "TextInBox.hpp"
#include "Look/WindArrowLook.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Math/Angle.hpp"
#include "Math/Screen.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>
#include <cstdio>

void
WindArrowRenderer::DrawArrow(Canvas &canvas, RasterPoint pos, Angle angle,
                             PixelScalar length, WindArrowStyle arrow_style,
                             PixelScalar offset)
{
  // Draw arrow

  RasterPoint arrow[] = {
    { 0, (PixelScalar)(-offset + 3) },
    { -6, (PixelScalar)(-offset - 3 - length) },
    { 0, (PixelScalar)(-offset + 3 - length) },
    { 6, (PixelScalar)(-offset - 3 - length) },
  };

  // Rotate the arrow
  PolygonRotateShift(arrow, ARRAY_SIZE(arrow), pos.x, pos.y, angle);

  canvas.Select(look.arrow_pen);
  canvas.Select(look.arrow_brush);
  canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));

  // Draw arrow tail

  if (arrow_style == WindArrowStyle::FULL_ARROW) {
    RasterPoint tail[] = {
      { 0, (PixelScalar)(-offset + 3) },
      { 0, (PixelScalar)(-offset - 3 - min(PixelScalar(20), length) * 3) },
    };

    PolygonRotateShift(tail, ARRAY_SIZE(tail),
                       pos.x, pos.y, angle);

    canvas.Select(look.tail_pen);
    canvas.DrawLine(tail[0], tail[1]);
  }
}

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const SpeedVector wind, const RasterPoint pos,
                        const PixelRect rc, WindArrowStyle arrow_style)
{
  // Draw arrow (and tail)

  PixelScalar length = iround(4 * wind.norm);
  DrawArrow(canvas, pos, wind.bearing - screen_angle, length, arrow_style);

  // Draw wind speed label

  StaticString<12> buffer;
  buffer.Format(_T("%i"), iround(Units::ToUserWindSpeed(wind.norm)));

  canvas.SetTextColor(COLOR_BLACK);
  canvas.Select(*look.font);

  PixelScalar offset = iround(fixed_sqrt_two * wind.norm);
  RasterPoint label[] = {
      { 18, (PixelScalar)(-26 - offset) },
  };
  PolygonRotateShift(label, ARRAY_SIZE(label),
                     pos.x, pos.y, wind.bearing - screen_angle);

  TextInBoxMode style;
  style.align = TextInBoxMode::Alignment::CENTER;
  style.vertical_position = TextInBoxMode::VerticalPosition::CENTERED;
  style.shape = LabelShape::OUTLINED;

  TextInBox(canvas, buffer, label[0].x, label[0].y, style, rc);
}

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const RasterPoint pos, const PixelRect rc,
                        const DerivedInfo &calculated,
                        const MapSettings &settings)
{
  if (!calculated.wind_available ||
      settings.wind_arrow_style == WindArrowStyle::NO_ARROW)
    return;

  // don't bother drawing it if not significant
  if (calculated.wind.norm < fixed_one)
    return;

  WindArrowRenderer::Draw(canvas, screen_angle, calculated.wind, pos, rc,
                          settings.wind_arrow_style);
}
