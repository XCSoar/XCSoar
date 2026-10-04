// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Engine/Airspace/Predicate/AirspacePredicate.hpp"
#include "util/StaticArray.hxx"
#include "Geo/GeoPoint.hpp"

#ifndef ENABLE_OPENGL
#include "TransparentRendererCache.hpp"
#include "util/Serial.hpp"
#else
#include "util/AllocatedArray.hxx"
#include "Math/Point2D.hpp"
#include <cstdint>
#include <unordered_map>
#endif

struct AirspaceLook;
struct MoreData;
struct DerivedInfo;
struct AirspaceComputerSettings;
struct AirspaceRendererSettings;
class AbstractAirspace;
class AirspacePolygon;
class Airspaces;
class ProtectedAirspaceWarningManager;
class AirspaceWarningCopy;
class Canvas;
class WindowProjection;
class MapCanvas;

class AirspaceRenderer
{
  const AirspaceLook &look;

  const Airspaces *airspaces = nullptr;
  const ProtectedAirspaceWarningManager *warning_manager = nullptr;

  StaticArray<GeoPoint,32> intersections;

#ifndef ENABLE_OPENGL
  /**
   * This object caches the airspace fill.  This avoids drawing it
   * again and again each frame when nothing has changed.
   */
  TransparentRendererCache fill_cache;

  Serial last_warning_serial;
#else
  /**
   * Geographic ear-clip indices per airspace.  Pan only reprojects.
   * A failed clip is remembered so we do not retry every frame.
   */
  struct TriangleCacheEntry {
    AllocatedArray<uint16_t> indices;
    unsigned index_count = 0;
    unsigned vertex_count = 0;
    bool tried = false;
    bool ok = false;
  };

  std::unordered_map<const AbstractAirspace *, TriangleCacheEntry>
    triangle_cache;

public:
  /**
   * Ring in native lon/lat offsets from #origin.  Drawn with
   * #ToGLM so pan/rotate does not ClipPolygon or ear-clip.
   */
  struct GeoPolyCache {
    GeoPoint origin = GeoPoint::Invalid();
    AllocatedArray<FloatPoint2D> ring;
    unsigned n = 0;
    AllocatedArray<FloatPoint2D> padding;
    unsigned padding_n = 0;
    double pad_scale = 0;
    double pad_cos = 0;
  };

private:
  std::unordered_map<const AbstractAirspace *, GeoPolyCache>
    geo_poly_cache;
#endif

public:
  AirspaceRenderer(const AirspaceLook &_look)
    :look(_look) {}

  const AirspaceLook &GetLook() const {
    return look;
  }

  const Airspaces *GetAirspaces() const {
    return airspaces;
  }

  const ProtectedAirspaceWarningManager *GetWarningManager() const {
    return warning_manager;
  }

  void SetAirspaces(const Airspaces *_airspaces) {
    if (airspaces != _airspaces)
      Flush();
    airspaces = _airspaces;
  }

  void SetAirspaceWarnings(const ProtectedAirspaceWarningManager *_warning_manager) {
    warning_manager = _warning_manager;
  }

  void Clear() {
    airspaces = nullptr;
    warning_manager = nullptr;
    Flush();
  }

  void Flush() {
#ifndef ENABLE_OPENGL
    fill_cache.Invalidate();
#else
    triangle_cache.clear();
    geo_poly_cache.clear();
#endif
  }

#ifdef ENABLE_OPENGL
  bool PrepareCachedPolygon(MapCanvas &canvas,
                            const AirspacePolygon &airspace,
                            bool skip_triangulate) noexcept;

  GeoPolyCache &GetGeoPoly(const AbstractAirspace &airspace) noexcept {
    return geo_poly_cache[&airspace];
  }
#endif

private:
#ifndef ENABLE_OPENGL
  bool DrawFill(Canvas &buffer_canvas, Canvas &stencil_canvas,
                const WindowProjection &projection,
                const AirspaceRendererSettings &settings,
                const AirspaceWarningCopy &awc,
                const AirspacePredicate &visible);

  void DrawFillCached(Canvas &canvas,
                      Canvas &stencil_canvas,
                      const WindowProjection &projection,
                      const AirspaceRendererSettings &settings,
                      const AirspaceWarningCopy &awc,
                      const AirspacePredicate &visible);

  void DrawOutline(Canvas &canvas,
                   const WindowProjection &projection,
                   const AirspaceRendererSettings &settings,
                   const AirspacePredicate &visible) const;
#endif

  void DrawInternal(Canvas &canvas,
#ifndef ENABLE_OPENGL
                    Canvas &stencil_canvas,
#endif
                    const WindowProjection &projection,
                    const AirspaceRendererSettings &settings,
                    const AirspaceWarningCopy &awc,
                    const AirspacePredicate &visible);

public:
  /**
   * Draw airspaces selected by the given #AirspacePredicate.
   */
  void Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
            Canvas &stencil_canvas,
#endif
            const WindowProjection &projection,
            const AirspaceRendererSettings &settings,
            const AirspaceWarningCopy &awc,
            const AirspacePredicate &visible);

  /**
   * Draw all airspaces.
   */
  void Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
            Canvas &stencil_canvas,
#endif
            const WindowProjection &projection,
            const AirspaceRendererSettings &settings);

  /**
   * Draw airspaces that are visible according to standard rules.
   */
  void Draw(Canvas &canvas,
#ifndef ENABLE_OPENGL
            Canvas &stencil_canvas,
#endif
            const WindowProjection &projection,
            const MoreData &basic, const DerivedInfo &calculated,
            const AirspaceComputerSettings &computer_settings,
            const AirspaceRendererSettings &settings);

  void DrawIntersections(Canvas &canvas,
                         const WindowProjection &projection) const;
};
