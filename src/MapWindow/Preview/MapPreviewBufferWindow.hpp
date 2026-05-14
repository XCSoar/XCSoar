// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapPreviewLayers.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Renderer/LabelBlock.hpp"
#include "ui/window/BufferWindow.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/BufferCanvas.hpp"
#endif

struct AirspaceLook;
struct TopographyLook;
class Canvas;

/**
 * Buffered base for inset map views under #MapWindow/Preview: shared terrain,
 * topography, airspace layers, geo projection, optional airspace stencil
 * (non-OpenGL), label block.
 *
 * Ground-layer behaviour aligns with #MapGroundLayers on the main map
 * (#MapWindowRender); previews share #MapPreviewLayers here.  Derived classes
 * add projection policy (#MapPreviewWindow focus vs #TargetMapWindow target)
 * and chrome (task, drag, overlays).
 *
 * Small-map seam inventory (UI-thread, read #CommonInterface unless noted):
 * - #MapPreviewWindow: #MapPreviewFocus-driven projection; terrain/topo/airspace
 *   via #PaintTerrainAndTopography / #PaintAirspace (airspace always); optional
 *   #PaintAirspaceMovingMap (task, waypoints, topo labels, aircraft) when
 *   airspace chrome enabled; high-res terrain via #MapPreviewLayers.
 * - #TargetMapWindow: #SetTarget projection; same ground helpers; airspace gated
 *   by map settings; GL fade; task (#DrawActiveTaskForProjection), waypoints,
 *   trail, topo labels, aircraft, map scale; mouse drag for AAT target.
 * - #MapItemPreviewWindow / #TaskEditMapPreviewWindow /
 *   #AirspaceWarningPreviewWindow:
 *   derive from #MapPreviewWindow; dialog hosts use #WidgetMaximumSizeUnbounded
 *   for max layout size.
 */
class MapPreviewBufferWindow : public BufferWindow {
protected:
  MapPreviewLayers layers;
  MapWindowProjection projection;

#ifndef ENABLE_OPENGL
  BufferCanvas airspace_buffer;
#endif

  LabelBlock label_block;

  MapPreviewBufferWindow(const AirspaceLook &airspace_look,
                         const TopographyLook &topography_look) noexcept;

  void InitialiseAirspaceBuffer() noexcept;
  void DeinitialiseAirspaceBuffer() noexcept;
  void ResizePreviewProjection(PixelSize new_size) noexcept;

  void PaintTerrainAndTopography(Canvas &canvas) noexcept;
  void PaintAirspace(Canvas &canvas, bool enabled) noexcept;

public:
  ~MapPreviewBufferWindow() noexcept override;
};
