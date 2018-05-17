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

#include "AirspaceListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Geo/GeoVector.hpp"
#include "Util/StaticString.hxx"

static void
Draw(Canvas &canvas, PixelRect rc,
     const AbstractAirspace &airspace,
     const TCHAR *comment,
     const TwoTextRowsRenderer &row_renderer,
     const AirspaceLook &look,
     const AirspaceRendererSettings &renderer_settings)
{
  const unsigned padding = Layout::GetTextPadding();
  const unsigned line_height = rc.GetHeight();

  const PixelPoint pt(rc.left + line_height / 2,
                      rc.top + line_height / 2);
  const unsigned radius = line_height / 2 - padding;
  AirspacePreviewRenderer::Draw(canvas, airspace, pt, radius,
                                renderer_settings, look);

  rc.left += line_height + padding;

  // Draw upper airspace altitude limit
  TCHAR buffer[40];
  AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetTop());
  const int top_x = row_renderer.DrawRightFirstRow(canvas, rc, buffer);

  // Draw lower airspace altitude limit
  AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetBase());
  const int bottom_x = row_renderer.DrawRightSecondRow(canvas, rc, buffer);

  rc.right = std::min(top_x, bottom_x);

  // Draw comment line
  row_renderer.DrawSecondRow(canvas, rc, comment);

  // Draw airspace name
  row_renderer.DrawFirstRow(canvas, rc, airspace.GetName());
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const TwoTextRowsRenderer &row_renderer,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  ::Draw(canvas, rc, airspace, AirspaceFormatter::GetClass(airspace),
         row_renderer, look, renderer_settings);
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const GeoVector &vector,
                           const TwoTextRowsRenderer &row_renderer,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  StaticString<256> comment(AirspaceFormatter::GetClass(airspace));

  comment.AppendFormat(_T(" - %s - %s"),
                       FormatUserDistanceSmart(vector.distance).c_str(),
                       FormatBearing(vector.bearing).c_str());

  ::Draw(canvas, rc, airspace, comment,
         row_renderer, look, renderer_settings);
}
