// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningPreviewWindow.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Interface.hpp"
#include "Look/MapLook.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "ui/canvas/Canvas.hpp"

#include <variant>

AirspaceWarningPreviewWindow::AirspaceWarningPreviewWindow(
  const MapLook &map_look,
  ProtectedAirspaceWarningManager &warnings) noexcept
  : MapPreviewWindow(map_look.airspace, map_look.topography, &warnings)
{
  SetAirspacePreviewMapLook(&map_look);
}

AirspaceWarningPreviewWindow::~AirspaceWarningPreviewWindow() noexcept = default;

void
AirspaceWarningPreviewWindow::SetHighlight(ConstAirspacePtr airspace) noexcept
{
  if (airspace)
    SetPreviewFocus(std::move(airspace));
  else
    SetPreviewFocus(std::monostate{});
}

void
AirspaceWarningPreviewWindow::PaintOverlays(Canvas &canvas) noexcept
{
  if (const auto *p = std::get_if<ConstAirspacePtr>(&GetFocus()))
    if (*p)
      AirspaceRenderer::DrawOutlineHighlight(
        canvas, GetProjection(), *(*p),
        CommonInterface::GetMapSettings().airspace);
}
