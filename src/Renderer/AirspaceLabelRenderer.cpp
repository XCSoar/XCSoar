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

#include "AirspaceLabelList.hpp"
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
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Sizes.h"

class AirspaceMapVisible : public AirspacePredicate
{
  const AirspaceVisibility visible_predicate;
  const AirspaceWarningCopy &warnings;

public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState& _state,
                     const AirspaceWarningCopy& _warnings)
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warnings(_warnings) {}

  bool operator()(const AbstractAirspace& airspace) const {
    return visible_predicate(airspace) ||
      warnings.IsInside(airspace) ||
      warnings.HasWarning(airspace);
  }
};

void
AirspaceLabelRenderer::Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
                            Canvas &stencil_canvas,
#endif
                            const WindowProjection &projection,
                            const MoreData &basic, const DerivedInfo &calculated,
                            const AirspaceComputerSettings &computer_settings,
                            const AirspaceRendererSettings &settings)
{
  if (airspaces == nullptr || airspaces->IsEmpty())
    return;

  AirspaceWarningCopy awc;
  if (warning_manager != nullptr)
    awc.Visit(*warning_manager);

  const AircraftState aircraft = ToAircraftState(basic, calculated);
  const AirspaceMapVisible visible(computer_settings, settings,
                                   aircraft, awc);

  DrawInternal(canvas,
#ifndef ENABLE_OPENGL
               stencil_canvas,
#endif
               projection, settings, awc, visible, computer_settings.warnings);
}

void
AirspaceLabelRenderer::DrawInternal(Canvas &canvas,
#ifndef ENABLE_OPENGL
                                    Canvas &stencil_canvas,
#endif
                                    const WindowProjection &projection,
                                    const AirspaceRendererSettings &settings,
                                    const AirspaceWarningCopy &awc,
                                    const AirspacePredicate &visible,
                                    const AirspaceWarningConfig &config)
{
  AirspaceLabelList labels;
  for (const auto &i : airspaces->QueryWithinRange(projection.GetGeoScreenCenter(),
                                                   projection.GetScreenDistanceMeters())) {
    const AbstractAirspace &airspace = i.GetAirspace();
    if (visible(airspace))
      labels.Add(airspace.GetCenter(), airspace.GetType(), airspace.GetBase(),
                 airspace.GetTop());
  }

  if(settings.label_selection == AirspaceRendererSettings::LabelSelection::ALL)
  {
    labels.Sort(config);

    // default paint settings
    canvas.SetTextColor(look.label_text_color);
    canvas.Select(*look.name_font);
    canvas.Select(look.label_pen);
    canvas.Select(look.label_brush);
    canvas.SetBackgroundTransparent();

    // draw
    TCHAR topText[NAME_SIZE + 1];
    TCHAR baseText[NAME_SIZE + 1];

    for (const auto &label : labels) {
      // size of text
      AirspaceFormatter::FormatAltitudeShort(topText, label.top, false);
      PixelSize topSize = canvas.CalcTextSize(topText);
      AirspaceFormatter::FormatAltitudeShort(baseText, label.base, false);
      PixelSize baseSize = canvas.CalcTextSize(baseText);
      int labelWidth = std::max(topSize.cx, baseSize.cx) +
                       2 * Layout::GetTextPadding();
      int labelHeight = topSize.cy + baseSize.cy;

      // box
      const auto pos = projection.GeoToScreen(label.pos);
      PixelRect rect;
      rect.left = pos.x - labelWidth / 2;
      rect.top = pos.y;
      rect.right = rect.left + labelWidth;
      rect.bottom = rect.top + labelHeight;
      canvas.Rectangle(rect.left, rect.top, rect.right, rect.bottom);

#ifdef USE_GDI
      canvas.DrawLine(rect.left + Layout::GetTextPadding(),
                      rect.top + labelHeight / 2,
                      rect.right - Layout::GetTextPadding(),
                      rect.top + labelHeight / 2);
#else
      canvas.DrawHLine(rect.left + Layout::GetTextPadding(),
                       rect.right - Layout::GetTextPadding(),
                       rect.top + labelHeight / 2, look.label_pen.GetColor());
#endif

      // top text
      int x = rect.right - Layout::GetTextPadding() - topSize.cx;
      int y = rect.top;
      canvas.DrawText(x, y, topText);

      // base text
      x = rect.right - Layout::GetTextPadding() - baseSize.cx;
      y = rect.bottom - baseSize.cy;
      canvas.DrawText(x, y, baseText);
    }
  }
}
