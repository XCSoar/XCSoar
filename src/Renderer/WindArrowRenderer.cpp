/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Math/Angle.hpp"
#include "Math/Util.hpp"
#include "Math/Screen.hpp"
#include "NMEA/Derived.hpp"
#include "Units/Units.hpp"
#include "Util/Macros.hpp"

#include <tchar.h>

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scope.hpp"
#endif

void
WindArrowRenderer::DrawArrow(Canvas &canvas, PixelPoint pos, Angle angle,
                             unsigned length, WindArrowStyle arrow_style,
                             int offset)
{
  // Draw arrow

  BulkPixelPoint arrow[] = {
    { 0, -offset + 3 },
    { -6, -offset - 3 - int(length) },
    { 0, -offset + 3 - int(length) },
    { 6, -offset - 3 - int(length) },
  };

  // Rotate the arrow
  PolygonRotateShift(arrow, ARRAY_SIZE(arrow), pos, angle);

  canvas.Select(look.arrow_pen);
  canvas.Select(look.arrow_brush);
  {
#ifdef ENABLE_OPENGL
    const ScopeAlphaBlend alpha_blend;
#endif
    canvas.DrawPolygon(arrow, ARRAY_SIZE(arrow));
  }

  // Draw arrow tail

  if (arrow_style == WindArrowStyle::FULL_ARROW) {
    BulkPixelPoint tail[] = {
      { 0, -offset + 3 },
      { 0, -offset - 3 - int(std::min(20u, length) * 3u) },
    };

    PolygonRotateShift(tail, ARRAY_SIZE(tail),
                       pos, angle);

    canvas.Select(look.tail_pen);
    canvas.DrawLine(tail[0], tail[1]);
  }
}

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const SpeedVector wind, const PixelPoint pos,
                        const PixelRect rc, WindArrowStyle arrow_style)
{
  // Draw arrow (and tail)

  const unsigned length = uround(4 * wind.norm);
  DrawArrow(canvas, pos, wind.bearing - screen_angle, length, arrow_style);

  // Draw wind speed label

  StaticString<12> buffer;
  buffer.Format(_T("%i"), iround(Units::ToUserWindSpeed(wind.norm)));

  canvas.SetTextColor(COLOR_BLACK);
  canvas.Select(*look.font);

  const unsigned offset = uround(M_SQRT2 * wind.norm);
  BulkPixelPoint label[] = {
    { 18, -26 - int(offset) },
  };
  PolygonRotateShift(label, ARRAY_SIZE(label),
                     pos, wind.bearing - screen_angle);

  TextInBoxMode style;
  style.align = TextInBoxMode::Alignment::CENTER;
  style.vertical_position = TextInBoxMode::VerticalPosition::CENTERED;
  style.shape = LabelShape::OUTLINED;

  TextInBox(canvas, buffer, label[0].x, label[0].y, style, rc);
}

void
WindArrowRenderer::Draw(Canvas &canvas, const Angle screen_angle,
                        const PixelPoint pos, const PixelRect rc,
                        const DerivedInfo &calculated,
                        const MapSettings &settings)
{
  if (!calculated.wind_available ||
      settings.wind_arrow_style == WindArrowStyle::NO_ARROW)
    return;

  // don't bother drawing it if not significant
  if (calculated.wind.norm < 1)
    return;

  WindArrowRenderer::Draw(canvas, screen_angle, calculated.wind, pos, rc,
                          settings.wind_arrow_style);
}
