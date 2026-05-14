// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/AirspaceRenderer.hpp"
#include "Renderer/AirspaceLabelRenderer.hpp"

class Canvas;
class WindowProjection;
class LabelBlock;
struct MoreData;
struct DerivedInfo;
struct AirspaceComputerSettings;
struct AirspaceRendererSettings;

/**
 * Shared paint helpers for the main map (#MapWindowRender).  Inset map
 * previews use #MapPreviewLayers plus terrain via
 * #BackgroundRenderer::PaintTerrainLayer;
 * airspace fill/outline paths match #PaintAirspaceMapLayers here when labels are
 * wired from the caller.
 */
namespace MapGroundLayers {

/**
 * Draw topography vectors when enabled and the renderer exists.
 */
template<typename TR>
inline void
PaintTopographyLayer(TR *renderer, bool topography_enabled,
                     Canvas &canvas,
                     const WindowProjection &projection) noexcept
{
  if (renderer != nullptr && topography_enabled)
    renderer->Draw(canvas, projection);
}

/**
 * Draw topography labels when enabled and the renderer exists.
 */
template<typename TR>
inline void
PaintTopographyLabelsLayer(TR *renderer, bool topography_enabled,
                           Canvas &canvas,
                           const WindowProjection &projection,
                           LabelBlock &label_block) noexcept
{
  if (renderer != nullptr && topography_enabled)
    renderer->DrawLabels(canvas, projection, label_block);
}

/**
 * Draw standard airspace fills/outlines, optionally followed by map labels.
 * @param labels nullptr skips the label pass (embedded previews).
 */
inline void
PaintAirspaceMapLayers(AirspaceRenderer &renderer,
                       AirspaceLabelRenderer *labels,
                       Canvas &canvas,
#ifndef ENABLE_OPENGL
                       Canvas &stencil_canvas,
#endif
                       const WindowProjection &projection,
                       const MoreData &basic, const DerivedInfo &calculated,
                       const AirspaceComputerSettings &computer_settings,
                       const AirspaceRendererSettings &settings) noexcept
{
  renderer.Draw(canvas,
#ifndef ENABLE_OPENGL
                stencil_canvas,
#endif
                projection, basic, calculated, computer_settings, settings);

  if (labels != nullptr)
    labels->Draw(canvas, projection, basic, calculated,
                 computer_settings, settings);
}

} // namespace MapGroundLayers
