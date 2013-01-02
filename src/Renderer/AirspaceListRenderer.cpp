/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/DialogLook.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Geo/GeoVector.hpp"
#include "Util/StaticString.hpp"
#include "Util/Macros.hpp"

namespace AirspaceListRenderer
{
  void Draw(Canvas &canvas, const PixelRect rc, const AbstractAirspace &airspace,
            const TCHAR *comment, const DialogLook &dialog_look,
            const AirspaceLook &look,
            const AirspaceRendererSettings &renderer_settings);
}

UPixelScalar
AirspaceListRenderer::GetHeight(const DialogLook &look)
{
  return look.list.font->GetHeight() + Layout::Scale(6) +
         look.small_font->GetHeight();
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const TCHAR *comment, const DialogLook &dialog_look,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  const PixelScalar line_height = rc.bottom - rc.top;

  const Font &name_font = *dialog_look.list.font;
  const Font &small_font = *dialog_look.small_font;

  PixelScalar left = rc.left + line_height + Layout::FastScale(2);
  canvas.Select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      airspace.GetName());

  canvas.Select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, comment);

  tstring top = AirspaceFormatter::GetTopShort(airspace);
  PixelScalar altitude_width =
    canvas.CalcTextWidth(top.c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.GetHeight() -
                      small_font.GetHeight() + Layout::FastScale(2), rc,
                      top.c_str());

  tstring base = AirspaceFormatter::GetBaseShort(airspace);
  altitude_width = canvas.CalcTextWidth(base.c_str());

  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.GetHeight() + Layout::FastScale(4),
                      rc, base.c_str());

  RasterPoint pt = { PixelScalar(rc.left + line_height / 2),
                     PixelScalar(rc.top + line_height / 2) };
  PixelScalar radius = std::min(PixelScalar(line_height / 2
                                            - Layout::FastScale(4)),
                                Layout::FastScale(10));
  AirspacePreviewRenderer::Draw(canvas, airspace, pt, radius,
                                renderer_settings, look);
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const DialogLook &dialog_look,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  Draw(canvas, rc, airspace, AirspaceFormatter::GetClass(airspace), dialog_look,
       look, renderer_settings);
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const GeoVector &vector,
                           const DialogLook &dialog_look,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  StaticString<256> comment(AirspaceFormatter::GetClass(airspace));

  TCHAR dist[20], bearing[20];
  FormatUserDistanceSmart(vector.distance, dist, true);
  FormatBearing(bearing, ARRAY_SIZE(bearing), vector.bearing);
  comment.AppendFormat(_T(" - %s - %s"), dist, bearing);

  Draw(canvas, rc, airspace, comment, dialog_look,
       look, renderer_settings);
}
