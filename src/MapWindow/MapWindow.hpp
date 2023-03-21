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

  const Waypoints *waypoints = nullptr;
  TopographyStore *topography = nullptr;
  CachedTopographyRenderer *topography_renderer = nullptr;

  RasterTerrain *terrain = nullptr;

  std::shared_ptr<RaspStore> rasp_store;

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
            const TrafficLook &traffic_look);
  virtual ~MapWindow();

  /**
   * Is the rendered map following the user's aircraft (i.e. near it)?
   */
  bool IsNearSelf() const {
    return follow_mode == FOLLOW_SELF;
  }

  bool IsPanning() const {
    return follow_mode == FOLLOW_PAN;
  }

  void SetWaypoints(const Waypoints *_waypoints) {
    waypoints = _waypoints;
    waypoint_renderer.SetWaypoints(waypoints);
  }

  void SetTask(ProtectedTaskManager *_task) {
    task = _task;
  }

  void SetRoutePlanner(const ProtectedRoutePlanner *_route_planner) {
    route_planner = _route_planner;
  }

  void SetGlideComputer(GlideComputer *_gc);

  void SetAirspaces(Airspaces *airspaces) {
    airspace_renderer.SetAirspaces(airspaces);
    airspace_label_renderer.SetAirspaces(airspaces);
  }

  void SetTopography(TopographyStore *_topography);
  void SetTerrain(RasterTerrain *_terrain);

  const std::shared_ptr<RaspStore> &GetRasp() const {
    return rasp_store;
  }

  void SetRasp(const std::shared_ptr<RaspStore> &_rasp_store);

#ifdef ENABLE_OPENGL
  void SetOverlay(std::unique_ptr<MapOverlay> &&_overlay);

  const MapOverlay *GetOverlay() const {
    return overlay.get();
  }
#endif

#ifdef HAVE_NOAA
  void SetNOAAStore(NOAAStore *_noaa_store) {
    noaa_store = _noaa_store;
  }
#endif

#ifdef HAVE_SKYLINES_TRACKING
  void SetSkyLinesData(const SkyLinesTracking::Data *_data) {
    skylines_data = _data;
  }
#endif

#ifdef HAVE_HTTP
  void SetThermalInfoMap(const TIM::Glue *_tim) {
    tim_glue = _tim;
  }
#endif

  void FlushCaches();

  using MapWindowBlackboard::ReadBlackboard;

  void ReadBlackboard(const MoreData &nmea_info,
                      const DerivedInfo &derived_info,
                      const ComputerSettings &settings_computer,
                      const MapSettings &settings_map);

  const MapWindowProjection &VisibleProjection() const {
    return visible_projection;
  }

  [[gnu::pure]]
  GeoPoint GetLocation() const {
    return visible_projection.IsValid()
      ? visible_projection.GetGeoLocation()
      : GeoPoint::Invalid();
  }

  void SetLocation(const GeoPoint location) {
    visible_projection.SetGeoLocation(location);
  }

  void UpdateScreenBounds() {
    visible_projection.UpdateScreenBounds();
  }

protected:
  void DrawBestCruiseTrack(Canvas &canvas, PixelPoint aircraft_pos) const;
  void DrawTrackBearing(Canvas &canvas,
                        PixelPoint aircraft_pos, bool circling) const;
  void DrawCompass(Canvas &canvas, const PixelRect &rc) const;
  void DrawWind(Canvas &canvas, const PixelPoint &Orig,
                           const PixelRect &rc) const;
  void DrawWaypoints(Canvas &canvas);

  void DrawTrail(Canvas &canvas, PixelPoint aircraft_pos,
                 TimeStamp min_time, bool enable_traildrift = false) noexcept;
  virtual void RenderTrail(Canvas &canvas, PixelPoint aircraft_pos);
  virtual void RenderTrackBearing(Canvas &canvas, PixelPoint aircraft_pos);

#ifdef HAVE_SKYLINES_TRACKING
  void DrawSkyLinesTraffic(Canvas &canvas) const;
#endif

  void DrawTeammate(Canvas &canvas) const;
  void DrawContest(Canvas &canvas);
  void DrawTask(Canvas &canvas);
  void DrawRoute(Canvas &canvas);
  void DrawTaskOffTrackIndicator(Canvas &canvas);
  void DrawWaves(Canvas &canvas);
  virtual void DrawThermalEstimate(Canvas &canvas) const;

  void DrawGlideThroughTerrain(Canvas &canvas) const;
  void DrawTerrainAbove(Canvas &canvas);
  void DrawFLARMTraffic(Canvas &canvas, PixelPoint aircraft_pos) const;
  void DrawGLinkTraffic(Canvas &canvas, PixelPoint aircraft_pos) const;

  // thread, main functions
  /**
   * Renders all the components of the moving map
   * @param canvas The drawing canvas
   * @param rc The area to draw in
   */
  virtual void Render(Canvas &canvas, const PixelRect &rc);

  unsigned UpdateTopography(unsigned max_update=1024);

  /**
   * @return true if UpdateTerrain() should be called again
   */
  bool UpdateTerrain();

  void UpdateAll() {
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
  void RenderTerrain(Canvas &canvas);

  void RenderRasp(Canvas &canvas);

  void RenderTerrainAbove(Canvas &canvas, bool working);

  /**
   * Renders the topography
   * @param canvas The drawing canvas
   */
  void RenderTopography(Canvas &canvas);
  /**
   * Renders the topography labels
   * @param canvas The drawing canvas
   */
  void RenderTopographyLabels(Canvas &canvas);

  void RenderOverlays(Canvas &canvas);

  /**
   * Renders the final glide shading
   * @param canvas The drawing canvas
   */
  void RenderFinalGlideShading(Canvas &canvas);
  /**
   * Renders the airspace
   * @param canvas The drawing canvas
   */
  void RenderAirspace(Canvas &canvas);

  /**
   * Renders the NOAA stations
   * @param canvas The drawing canvas
   */
  void RenderNOAAStations(Canvas &canvas);
  /**
   * Render final glide through terrain marker
   * @param canvas The drawing canvas
   */
  void RenderGlide(Canvas &canvas);

public:
  void SetMapScale(const double x) {
    visible_projection.SetMapScale(x);
  }
};
