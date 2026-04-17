// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Look/Colors.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Geo/GeoVector.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

static void
Draw(Canvas &canvas, PixelRect rc,
     const AbstractAirspace &airspace,
     const char *comment,
     const TwoTextRowsRenderer &row_renderer,
     const AirspaceLook &look,
     const AirspaceRendererSettings &renderer_settings,
     bool cleared)
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
  char buffer[40];
  AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetTop());
  const int top_x = row_renderer.DrawRightFirstRow(canvas, rc, buffer);

  // Draw lower airspace altitude limit
  AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetBase());
  const int bottom_x = row_renderer.DrawRightSecondRow(canvas, rc, buffer);

  int right_column_x = std::min(top_x, bottom_x);

  // Draw "[Cleared]" tag left of the altitude column
  if (cleared) {
    const Color saved_color = canvas.GetTextColor();
    canvas.SetTextColor(COLOR_CLEARANCE);
    const char *tag = _("Cleared");
    StaticString<32> tag_text;
    tag_text.Format("[%s]", tag);
    PixelRect tag_rc = rc;
    tag_rc.right = right_column_x;
    right_column_x =
      row_renderer.DrawRightSecondRow(canvas, tag_rc, tag_text);
    canvas.SetTextColor(saved_color);
  }

  rc.right = right_column_x;

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
                           const AirspaceRendererSettings &renderer_settings,
                           bool cleared)
{
  ::Draw(canvas, rc, airspace,
         AirspaceFormatter::GetClassOrType(airspace),
         row_renderer, look, renderer_settings, cleared);
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const GeoVector &vector,
                           const TwoTextRowsRenderer &row_renderer,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings,
                           bool cleared)
{
  StaticString<256> comment(
    AirspaceFormatter::GetClassOrType(airspace));

  comment.AppendFormat(" - %s - %s",
                       FormatUserDistanceSmart(vector.distance).c_str(),
                       FormatBearing(vector.bearing).c_str());

  ::Draw(canvas, rc, airspace, comment,
         row_renderer, look, renderer_settings, cleared);
}
