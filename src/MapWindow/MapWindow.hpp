// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Projection/MapWindowProjection.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "ui/window/DoubleBufferWindow.hpp"
#ifndef ENABLE_OPENGL
#include "ui/canvas/BufferCanvas.hpp"
#endif
#include "Renderer/LabelBlock.hpp"
#include "Screen/StopWatch.hpp"
#include "MapWindowBlackboard.hpp"
#include "Renderer/AirspaceLabelRenderer.hpp"
#include "Renderer/BackgroundRenderer.hpp"
#include "Renderer/WaypointRenderer.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "Weather/Features.hpp"
#include "Tracking/SkyLines/Features.hpp"

#include <memory>

struct MapLook;
struct TrafficLook;
class TopographyStore;
class CachedTopographyRenderer;
class RasterTerrain;
class RaspStore;
class RaspRenderer;
class Skysight;
class MapOverlay;
class Waypoints;
class Airspaces;
class ProtectedTaskManager;
class GlideComputer;
class ContainerWindow;
class NOAAStore;
class MapOverlay;

namespace SkyLinesTracking {
  struct Data;
}

namespace TIM { class Glue; }

class MapWindow :
  public DoubleBufferWindow,
  public MapWindowBlackboard
{
#ifndef ENABLE_OPENGL
  // graphics vars

  BufferCanvas buffer_canvas;
#endif

  LabelBlock label_block;

protected:
  const MapLook &look;

  /**
   * What object does the projection's screen origin follow?
   */
  enum FollowMode {
    /**
     * Follow the user's aircraft.
     */
    FOLLOW_SELF,

    /**
     * The user pans the map.
     */
    FOLLOW_PAN,
  };

  FollowMode follow_mode = FOLLOW_SELF;

  /**
   * The projection as currently visible on the screen.  This object
   * is being edited by the user.
   */
  MapWindowProjection visible_projection;

#ifndef ENABLE_OPENGL
  /**
   * The projection of the buffer.  This differs from
   * visible_projection only after the projection was modified, until
   * the DrawThread has finished drawing the new projection.
   */
  MapWindowProjection buffer_projection;
#endif

  /**
   * The projection of the DrawThread.  This is used to render the new
   * map.  After rendering has completed, this object is copied over
   * #buffer_projection.
   */
  MapWindowProjection render_projection;

  Waypoints *waypoints = nullptr;
  TopographyStore *topography = nullptr;
  CachedTopographyRenderer *topography_renderer = nullptr;

  RasterTerrain *terrain = nullptr;

  std::shared_ptr<RaspStore> rasp_store;
  std::shared_ptr<Skysight> skysight;

  /**
   * The current RASP renderer.  Modifications to this pointer (but
   * not to the #RaspRenderer instance) are protected by
   * #DoubleBufferWindow::mutex.
   */
  std::unique_ptr<RaspRenderer> rasp_renderer;

#ifdef ENABLE_OPENGL
  std::unique_ptr<MapOverlay> overlay;
#endif

  const TrafficLook &traffic_look;

  BackgroundRenderer background;
  WaypointRenderer waypoint_renderer;

  AirspaceRenderer airspace_renderer;
  AirspaceLabelRenderer airspace_label_renderer;

  TrailRenderer trail_renderer;

  ProtectedTaskManager *task = nullptr;
  const ProtectedRoutePlanner *route_planner = nullptr;
  GlideComputer *glide_computer = nullptr;

#ifdef HAVE_NOAA
  NOAAStore *noaa_store = nullptr;
#endif

#ifdef HAVE_SKYLINES_TRACKING
  const SkyLinesTracking::Data *skylines_data = nullptr;
#endif

#ifdef HAVE_HTTP
  const TIM::Glue *tim_glue = nullptr;
#endif

  bool compass_visible = true;

#ifndef ENABLE_OPENGL
  /**
   * Tracks whether the buffer canvas contains valid data.  We use
   * those attributes to prevent showing invalid data on the map, when
   * the user switches quickly to or from full-screen mode.
   */
  unsigned ui_generation = 1, buffer_generation = 0;

  /**
   * If non-zero, then the buffer will be scaled to the new
   * projection, and this variable is decremented.  This is used while
   * zooming and panning, to give instant visual feedback.
   */
  unsigned scale_buffer = 0;
#endif

  /**
   * The #StopWatch used to benchmark the DrawThread,
   * i.e. OnPaintBuffer().
   */
  ScreenStopWatch draw_sw;

  friend class DrawThread;

public:
  MapWindow(const MapLook &look,
            const TrafficLook &traffic_look) noexcept;
  virtual ~MapWindow() noexcept;

  /**
   * Is the rendered map following the user's aircraft (i.e. near it)?
   */
  bool IsNearSelf() const noexcept {
    return follow_mode == FOLLOW_SELF;
  }

  bool IsPanning() const noexcept {
    return follow_mode == FOLLOW_PAN;
  }

  void SetWaypoints(Waypoints *_waypoints) noexcept {
    waypoints = _waypoints;
    waypoint_renderer.SetWaypoints(waypoints);
  }

  void SetTask(ProtectedTaskManager *_task) noexcept {
    task = _task;
  }

  void SetRoutePlanner(const ProtectedRoutePlanner *_route_planner) noexcept {
    route_planner = _route_planner;
  }

  void SetGlideComputer(GlideComputer *_gc) noexcept;

  void SetAirspaces(Airspaces *airspaces) noexcept {
    airspace_renderer.SetAirspaces(airspaces);
    airspace_label_renderer.SetAirspaces(airspaces);
  }

  void SetTopography(TopographyStore *_topography) noexcept;
  void SetTerrain(RasterTerrain *_terrain) noexcept;

  const std::shared_ptr<RaspStore> &GetRasp() const noexcept {
    return rasp_store;
  }

  const std::shared_ptr<Skysight> &GetSkysight() const noexcept{
    return skysight;
  }

  void SetRasp(const std::shared_ptr<RaspStore> &_rasp_store) noexcept;
  void SetSkysight(const std::shared_ptr<Skysight> &_skysight) noexcept;

#ifdef ENABLE_OPENGL
  void SetOverlay(std::unique_ptr<MapOverlay> &&_overlay) noexcept;

  const MapOverlay *GetOverlay() const noexcept {
    return overlay.get();
  }
#endif

#ifdef HAVE_NOAA
  void SetNOAAStore(NOAAStore *_noaa_store) noexcept {
    noaa_store = _noaa_store;
  }
#endif

#ifdef HAVE_SKYLINES_TRACKING
  void SetSkyLinesData(const SkyLinesTracking::Data *_data) noexcept {
    skylines_data = _data;
  }
#endif

#ifdef HAVE_HTTP
  void SetThermalInfoMap(const TIM::Glue *_tim) noexcept {
    tim_glue = _tim;
  }
#endif

  void FlushCaches() noexcept;

  using MapWindowBlackboard::ReadBlackboard;

  void ReadBlackboard(const MoreData &nmea_info,
                      const DerivedInfo &derived_info,
                      const ComputerSettings &settings_computer,
                      const MapSettings &settings_map) noexcept;

  const MapWindowProjection &VisibleProjection() const noexcept {
    return visible_projection;
  }

  [[gnu::pure]]
  GeoPoint GetLocation() const noexcept {
    return visible_projection.IsValid()
      ? visible_projection.GetGeoLocation()
      : GeoPoint::Invalid();
  }

  void SetLocation(const GeoPoint location) noexcept {
    visible_projection.SetGeoLocation(location);
  }

  void UpdateScreenBounds() noexcept {
    visible_projection.UpdateScreenBounds();
  }

protected:
  void DrawBestCruiseTrack(Canvas &canvas, PixelPoint aircraft_pos) const noexcept;
  void DrawTrackBearing(Canvas &canvas,
                        PixelPoint aircraft_pos, bool circling) const noexcept;
  void DrawCompass(Canvas &canvas, const PixelRect &rc) const noexcept;
  void DrawWind(Canvas &canvas, const PixelPoint &Orig,
                           const PixelRect &rc) const noexcept;
  void DrawWaypoints(Canvas &canvas) noexcept;

  void DrawTrail(Canvas &canvas, PixelPoint aircraft_pos,
                 TimeStamp min_time, bool enable_traildrift = false) noexcept;
  virtual void RenderTrail(Canvas &canvas, PixelPoint aircraft_pos) noexcept;
  virtual void RenderTrackBearing(Canvas &canvas, PixelPoint aircraft_pos) noexcept;

#ifdef HAVE_SKYLINES_TRACKING
  void DrawSkyLinesTraffic(Canvas &canvas) const noexcept;
#endif

  void DrawTeammate(Canvas &canvas) const noexcept;
  void DrawContest(Canvas &canvas) noexcept;
  void DrawTask(Canvas &canvas) noexcept;
  void DrawRoute(Canvas &canvas) noexcept;
  void DrawTaskOffTrackIndicator(Canvas &canvas) noexcept;
  void DrawWaves(Canvas &canvas) noexcept;
  virtual void DrawThermalEstimate(Canvas &canvas) const noexcept;

  void DrawGlideThroughTerrain(Canvas &canvas) const noexcept;
  void DrawTerrainAbove(Canvas &canvas) noexcept;
  void DrawFLARMTraffic(Canvas &canvas, PixelPoint aircraft_pos) const noexcept;
  void DrawGLinkTraffic(Canvas &canvas) const noexcept;

  // thread, main functions
  /**
   * Renders all the components of the moving map
   * @param canvas The drawing canvas
   * @param rc The area to draw in
   */
  virtual void Render(Canvas &canvas, const PixelRect &rc) noexcept;

  unsigned UpdateTopography(unsigned max_update=1024) noexcept;

  /**
   * @return true if UpdateTerrain() should be called again
   */
  bool UpdateTerrain() noexcept;

  void UpdateAll() noexcept {
    UpdateTopography();
    UpdateTerrain();
  }

protected:
  /* virtual methods from class Window */
  void OnCreate() override;
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;

#ifndef ENABLE_OPENGL
  void OnPaint(Canvas& canvas) noexcept override;
#endif

  /* methods from class DoubleBufferWindow */
  void OnPaintBuffer(Canvas& canvas) noexcept override;

private:
  /**
   * Renders the terrain background
   * @param canvas The drawing canvas
   */
  void RenderTerrain(Canvas &canvas) noexcept;

  void RenderRasp(Canvas &canvas) noexcept;

  void RenderTerrainAbove(Canvas &canvas, bool working) noexcept;

  /**
   * Renders the topography
   * @param canvas The drawing canvas
   */
  void RenderTopography(Canvas &canvas) noexcept;

  /**
   * Renders the topography labels
   * @param canvas The drawing canvas
   */
  void RenderTopographyLabels(Canvas &canvas) noexcept;

  void RenderOverlays(Canvas &canvas) noexcept;

  /**
   * Renders the final glide shading
   * @param canvas The drawing canvas
   */
  void RenderFinalGlideShading(Canvas &canvas) noexcept;

  /**
   * Renders the airspace
   * @param canvas The drawing canvas
   */
  void RenderAirspace(Canvas &canvas) noexcept;

  /**
   * Renders the NOAA stations
   * @param canvas The drawing canvas
   */
  void RenderNOAAStations(Canvas &canvas) noexcept;

  /**
   * Render final glide through terrain marker
   * @param canvas The drawing canvas
   */
  void RenderGlide(Canvas &canvas) noexcept;

public:
  void SetMapScale(const double x) noexcept {
    visible_projection.SetMapScale(x);
  }
};
