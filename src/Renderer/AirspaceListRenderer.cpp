// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Geo/GeoVector.hpp"
#include "util/StaticString.hxx"
#include "RadioFrequency.hpp"
#include "util/StringCompare.hxx"

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
  StaticString<256> comment(AirspaceFormatter::GetClassOrType(airspace));

  // Append frequency if defined
  const RadioFrequency frequency = airspace.GetRadioFrequency();
  if (frequency.IsDefined()) {
    TCHAR freq_buffer[16];
    if (frequency.Format(freq_buffer, sizeof(freq_buffer)) != nullptr) {
      comment.AppendFormat(_T(" - %s MHz"), freq_buffer);
    }
  }

  // Append station name if available
  const TCHAR *station_name = airspace.GetStationName();
  if (station_name != nullptr && !StringIsEmpty(station_name)) {
    comment.AppendFormat(_T(" - %s"), station_name);
  }

  ::Draw(canvas, rc, airspace, comment,
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
  StaticString<256> comment(AirspaceFormatter::GetClassOrType(airspace));

  // Append frequency if defined
  const RadioFrequency frequency = airspace.GetRadioFrequency();
  if (frequency.IsDefined()) {
    TCHAR freq_buffer[16];
    if (frequency.Format(freq_buffer, sizeof(freq_buffer)) != nullptr) {
      comment.AppendFormat(_T(" - %s MHz"), freq_buffer);
    }
  }

  // Append station name if available
  const TCHAR *station_name = airspace.GetStationName();
  if (station_name != nullptr && !StringIsEmpty(station_name)) {
    comment.AppendFormat(_T(" - %s"), station_name);
  }

  comment.AppendFormat(_T(" - %s - %s"),
                       FormatUserDistanceSmart(vector.distance).c_str(),
                       FormatBearing(vector.bearing).c_str());

  ::Draw(canvas, rc, airspace, comment,
         row_renderer, look, renderer_settings);
}
