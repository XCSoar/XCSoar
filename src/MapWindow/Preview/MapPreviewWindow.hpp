// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"
#include "Blackboard/RateLimitedBlackboardListener.hpp"
#include "Geo/GeoPoint.hpp"
#include "MapPreviewFocus.hpp"
#include "MapPreviewBufferWindow.hpp"

#include <chrono>

class Canvas;
class ContainerWindow;
class ProtectedAirspaceWarningManager;
struct AirspaceLook;
struct MapLook;
struct TopographyLook;

/**
 * #BufferWindow base for inset map previews in dialogs: shared layers,
 * projection, optional traffic tracking via #RateLimitedBlackboardListener.
 */
class MapPreviewWindow : public MapPreviewBufferWindow,
                         private NullBlackboardListener {
  MapPreviewFocus focus;
  GeoPoint query_fallback_location;

  RateLimitedBlackboardListener rate_limited_bl;
  bool blackboard_registered = false;

  /**
   * When set and focus is an airspace item, draw task / waypoints /
   * topography labels / aircraft like the main map (#PaintAirspaceMovingMap).
   */
  const MapLook *airspace_preview_map_look = nullptr;

protected:
  [[nodiscard]]
  const MapPreviewFocus &GetFocus() const noexcept {
    return focus;
  }

  [[nodiscard]]
  MapPreviewLayers &GetLayers() noexcept {
    return layers;
  }

  [[nodiscard]]
  MapWindowProjection &GetProjection() noexcept {
    return projection;
  }

public:
  MapPreviewWindow(const AirspaceLook &airspace_look,
                   const TopographyLook &topography_look,
                   ProtectedAirspaceWarningManager *warnings_manager =
                     nullptr) noexcept;

  ~MapPreviewWindow() noexcept override;

  MapPreviewWindow(const MapPreviewWindow &) = delete;
  MapPreviewWindow &operator=(const MapPreviewWindow &) = delete;

  void Create(ContainerWindow &parent, PixelRect rc,
              WindowStyle style) noexcept;

  void SetPreviewFocus(MapPreviewFocus f) noexcept;
  void SetQueryFallbackLocation(GeoPoint location) noexcept;

  void SetAirspacePreviewMapLook(const MapLook *map_look) noexcept {
    airspace_preview_map_look = map_look;
    SyncPreviewBlackboardListener();
    if (IsDefined())
      Invalidate();
  }

protected:
  void OnCreate() noexcept override;
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  void OnPaintBuffer(Canvas &canvas) noexcept override;

  virtual void PaintOverlays([[maybe_unused]] Canvas &canvas) noexcept {}

  virtual void UpdateProjection() noexcept;

  void ApplyFallbackProjection(const PixelRect &rc) noexcept;

private:
  void LinkDataSources() noexcept;
  void SyncPreviewBlackboardListener() noexcept;

  [[nodiscard]]
  bool HasAirspaceMovingMapChrome() const noexcept;

  void PaintAirspaceMovingMap(Canvas &canvas) noexcept;

  void OnGPSUpdate(const MoreData &basic) noexcept override;
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) noexcept override;
};
