// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Renderer/AirspaceRenderer.hpp"
#include "Renderer/BackgroundRenderer.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/BufferCanvas.hpp"
#endif

class Canvas;
class LabelBlock;
class MapWindowProjection;
class RasterTerrain;
class TopographyStore;
class TopographyRenderer;
class Airspaces;
class ProtectedAirspaceWarningManager;
struct AirspaceLook;
struct TopographyLook;

/**
 * Shared terrain / topography / airspace drawing for inset map previews
 * (#AirspaceWarningPreviewWindow, #TargetMapWindow, etc.). UI-thread only;
 * reads #CommonInterface inside render methods.
 */
class MapPreviewLayers {
  BackgroundRenderer background;
  TopographyRenderer *topography_renderer = nullptr;
  AirspaceRenderer airspace_renderer;

  const TopographyLook &topography_look;

  ProtectedAirspaceWarningManager *airspace_warnings = nullptr;

  bool data_linked = false;

public:
  MapPreviewLayers(const AirspaceLook &airspace_look,
                   const TopographyLook &_topography_look) noexcept;

  ~MapPreviewLayers() noexcept;

  MapPreviewLayers(const MapPreviewLayers &) = delete;
  MapPreviewLayers &operator=(const MapPreviewLayers &) = delete;

  [[nodiscard]]
  AirspaceRenderer &GetAirspaceRenderer() noexcept {
    return airspace_renderer;
  }

  void SetAirspaceWarnings(ProtectedAirspaceWarningManager *wm) noexcept;

  /**
   * Wire terrain / topography / airspace database once (#data_components).
   */
  void LinkFromDataSources() noexcept;

  void SetTerrain(RasterTerrain *terrain) noexcept {
    background.SetTerrain(terrain);
  }

  void SetTopography(TopographyStore *topography) noexcept;

  void RenderTerrain(Canvas &canvas,
                     const MapWindowProjection &projection) noexcept;

  void RenderTopography(Canvas &canvas,
                        const MapWindowProjection &projection) noexcept;

  void RenderTopographyLabels(Canvas &canvas,
                              const MapWindowProjection &projection,
                              LabelBlock &label_block) noexcept;

#ifndef ENABLE_OPENGL
  void RenderAirspace(Canvas &canvas,
                      BufferCanvas &stencil,
                      const MapWindowProjection &projection) noexcept;
#else
  void RenderAirspace(Canvas &canvas,
                      const MapWindowProjection &projection) noexcept;
#endif
};
