// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceListRenderer.hpp"
#include "TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Airspace/AbstractAirspace.hpp"
#include "Airspace/AirspaceClass.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"
#include "Geo/GeoVector.hpp"
#include "util/StaticString.hxx"

#include <cstring>
#include <string>

static std::string
NormalizeSingleLine(const char *text)
{
  std::string normalized;
  if (text == nullptr)
    return normalized;

  normalized.reserve(std::strlen(text));
  for (const char *i = text; *i != '\0'; ++i)
    normalized.push_back((*i == '\r' || *i == '\n') ? ' ' : *i);

  return normalized;
}

static void
Draw(Canvas &canvas, PixelRect rc,
     const AbstractAirspace &airspace,
     const char *primary_text, const char *secondary_text,
     const TwoTextRowsRenderer &row_renderer,
     const AirspaceLook &look,
     const AirspaceRendererSettings &renderer_settings)
{
  const char *safe_primary = primary_text != nullptr ? primary_text : "";
  const char *safe_secondary = secondary_text != nullptr ? secondary_text : "";

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

  rc.right = std::min(top_x, bottom_x);

  // Draw secondary row
  row_renderer.DrawSecondRow(canvas, rc, safe_secondary);

  // Draw primary row
  row_renderer.DrawFirstRow(canvas, rc, safe_primary);
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const TwoTextRowsRenderer &row_renderer,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  const char *class_or_type = AirspaceFormatter::GetClassOrType(airspace);
  const char *notam_type = AirspaceFormatter::GetType(airspace);
  const bool is_notam = airspace.GetType() == AirspaceClass::NOTAM;
  const auto normalized_name = NormalizeSingleLine(airspace.GetName());
  // NOTAMs: display type (e.g., "NOTAM") as primary, name as secondary
  // Others: display name as primary, class/type as secondary
  if (is_notam) {
    ::Draw(canvas, rc, airspace, notam_type, normalized_name.c_str(),
           row_renderer, look, renderer_settings);
  } else {
    ::Draw(canvas, rc, airspace, normalized_name.c_str(), class_or_type,
           row_renderer, look, renderer_settings);
  }
}

void
AirspaceListRenderer::Draw(Canvas &canvas, const PixelRect rc,
                           const AbstractAirspace &airspace,
                           const GeoVector &vector,
                           const TwoTextRowsRenderer &row_renderer,
                           const AirspaceLook &look,
                           const AirspaceRendererSettings &renderer_settings)
{
  const char *class_or_type = AirspaceFormatter::GetClassOrType(airspace);
  const char *notam_type = AirspaceFormatter::GetType(airspace);
  const bool is_notam = airspace.GetType() == AirspaceClass::NOTAM;
  const auto normalized_name = NormalizeSingleLine(airspace.GetName());
  const char *base_type = is_notam
    ? notam_type
    : class_or_type;
  StaticString<256> type_with_location(base_type != nullptr ? base_type : "");

  type_with_location.AppendFormat(" - %s - %s",
                                  FormatUserDistanceSmart(vector.distance).c_str(),
                                  FormatBearing(vector.bearing).c_str());

  if (is_notam) {
    // NOTAMs: class/type + distance + bearing as primary, name as secondary
    ::Draw(canvas, rc, airspace, type_with_location, normalized_name.c_str(),
           row_renderer, look, renderer_settings);
  } else {
    ::Draw(canvas, rc, airspace, normalized_name.c_str(), type_with_location,
           row_renderer, look, renderer_settings);
  }
}
