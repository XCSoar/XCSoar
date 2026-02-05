// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceLabelRenderer.hpp"
#include "AirspaceRendererSettings.hpp"
#include "Projection/WindowProjection.hpp"
#include "Look/AirspaceLook.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/AirspaceWarningCopy.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "NMEA/Aircraft.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Sizes.h"

class AirspaceMapVisible
{
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningCopy &warnings;

public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState &_state,
                     const AirspaceWarningCopy &_warnings) noexcept
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings) {}

  [[gnu::pure]]
  bool operator()(const AbstractAirspace& airspace) const noexcept {
    return visible_predicate(airspace) ||
      warnings.IsInside(airspace) ||
      warnings.HasWarning(airspace);
  }
};

void
AirspaceLabelRenderer::Draw(Canvas &canvas,
                            const WindowProjection &projection,
                            const MoreData &basic, const DerivedInfo &calculated,
                            const AirspaceComputerSettings &computer_settings,
                            const AirspaceRendererSettings &settings) noexcept
{
  if (settings.label_selection != AirspaceRendererSettings::LabelSelection::ALL ||
      airspaces == nullptr || airspaces->IsEmpty())
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != nullptr)
    awc.Visit(*warning_manager);

  const AircraftState aircraft = ToAircraftState(basic, calculated);
  const AirspaceMapVisible visible(computer_settings, settings,
                                   aircraft, awc);

  DrawInternal(canvas,
               projection, visible, computer_settings.warnings);
}

inline void
AirspaceLabelRenderer::DrawInternal(Canvas &canvas,
                                    const WindowProjection &projection,
                                    AirspacePredicate visible,
                                    const AirspaceWarningConfig &config) noexcept
{
  AirspaceLabelList labels;
  for (const auto &i : airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                                   projection.GetScreenDistanceMeters())) {
    const AbstractAirspace &airspace = i.GetAirspace();
    if (visible(airspace))
      labels.Add(airspace.GetCenter(), airspace.GetClass(), airspace.GetBase(),
                 airspace.GetTop());
  }

  labels.Sort(config);

  // default paint settings
  canvas.SetTextColor(look.label_text_color);
  canvas.Select(*look.name_font);
  canvas.Select(look.label_pen);
  canvas.Select(look.label_brush);
  canvas.SetBackgroundTransparent();

  // draw
  for (const auto &label : labels)
    DrawLabel(canvas, projection, label);
}

inline void
AirspaceLabelRenderer::DrawLabel(Canvas &canvas,
                                 const WindowProjection &projection,
                                 const AirspaceLabelList::Label &label) noexcept
{
  char topText[NAME_SIZE + 1];
  AirspaceFormatter::FormatAltitudeShort(topText, label.top, false);
  const PixelSize topSize = canvas.CalcTextSize(topText);

  char baseText[NAME_SIZE + 1];
  AirspaceFormatter::FormatAltitudeShort(baseText, label.base, false);
  const PixelSize baseSize = canvas.CalcTextSize(baseText);

  const unsigned padding = Layout::GetTextPadding();
  const unsigned labelWidth =
    std::max(topSize.width, baseSize.width) + 2 * padding;
  const unsigned labelHeight = topSize.height + baseSize.height;

  // box
  const auto pos = projection.GeoToScreen(label.pos);
  PixelRect rect;
  rect.left = pos.x - labelWidth / 2;
  rect.top = pos.y;
  rect.right = rect.left + labelWidth;
  rect.bottom = rect.top + labelHeight;
  canvas.DrawRectangle(rect);

#ifdef USE_GDI
  canvas.DrawLine(rect.left + padding,
                  rect.top + labelHeight / 2,
                  rect.right - padding,
                  rect.top + labelHeight / 2);
#else
  canvas.DrawHLine(rect.left + padding,
                   rect.right - padding,
                   rect.top + labelHeight / 2, look.label_pen.GetColor());
#endif

  // top text
  canvas.DrawText(rect.GetTopRight().At(-int(padding + topSize.width),
                                        0),
                  topText);

  // base text
  canvas.DrawText(rect.GetBottomRight().At(-int(padding + baseSize.width),
                                           -(int)baseSize.height),
                  baseText);
}
