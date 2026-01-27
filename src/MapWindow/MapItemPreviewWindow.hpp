// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Projection/MapWindowProjection.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "ui/window/BufferWindow.hpp"
#include "Renderer/LabelBlock.hpp"
#include "Renderer/BackgroundRenderer.hpp"
#include "Renderer/WaypointRenderer.hpp"
#include "Renderer/TrailRenderer.hpp"
#include "MapWindow/Items/MapItem.hpp"

#ifndef ENABLE_OPENGL
#include "ui/canvas/BufferCanvas.hpp"
#endif

struct WaypointLook;
struct TaskLook;
struct AircraftLook;
struct TopographyLook;
struct OverlayLook;
struct TrafficLook;
class ContainerWindow;
class TopographyStore;
class TopographyRenderer;
class Waypoints;
class Airspaces;
class ProtectedTaskManager;
class GlideComputer;

class MapItemPreviewWindow : public BufferWindow {
  const TaskLook &task_look;
  const AircraftLook &aircraft_look;
  const TopographyLook &topography_look;
  const OverlayLook &overlay_look;
  const TrafficLook &traffic_look;

#ifndef ENABLE_OPENGL
  BufferCanvas buffer_canvas;
#endif

  MapWindowProjection projection;

  LabelBlock label_block;

  BackgroundRenderer background;
  TopographyRenderer *topography_renderer = nullptr;

  AirspaceRenderer airspace_renderer;

  WaypointRenderer way_point_renderer;

  TrailRenderer trail_renderer;

  ProtectedTaskManager *task = nullptr;
  const GlideComputer *glide_computer = nullptr;

  const MapItem *current_item = nullptr;
  const AbstractAirspace *highlight_airspace = nullptr;
  GeoPoint target_location; // For LOCATION items

public:
  MapItemPreviewWindow(const WaypointLook &waypoint_look,
                      const AirspaceLook &_airspace_look,
                      const TrailLook &_trail_look,
                      const TaskLook &_task_look,
                      const AircraftLook &_aircraft_look,
                      const TopographyLook &_topography_look,
                      const OverlayLook &_overlay_look,
                      const TrafficLook &_traffic_look) noexcept;
  virtual ~MapItemPreviewWindow() noexcept;

  void Create(ContainerWindow &parent, PixelRect rc, WindowStyle style) noexcept;

  void SetTerrain(RasterTerrain *terrain) noexcept;
  void SetTopograpgy(TopographyStore *topography) noexcept;

  void SetAirspaces(Airspaces *airspace_database) noexcept {
    airspace_renderer.SetAirspaces(airspace_database);
  }

  void SetWaypoints(const Waypoints *way_points) noexcept {
    way_point_renderer.SetWaypoints(way_points);
  }

  void SetTask(ProtectedTaskManager *_task) noexcept {
    task = _task;
  }

  void SetGlideComputer(const GlideComputer *_gc) noexcept {
    glide_computer = _gc;
  }

  void SetMapItem(const MapItem &item) noexcept;

private:
  void RenderTerrain(Canvas &canvas) noexcept;
  void RenderTopography(Canvas &canvas) noexcept;
  void RenderTopographyLabels(Canvas &canvas) noexcept;
  void RenderAirspace(Canvas &canvas) noexcept;
  void DrawHighlightedAirspace(Canvas &canvas) noexcept;
  void DrawAirspaceLabel(Canvas &canvas) noexcept;
  void RenderTrail(Canvas &canvas) noexcept;
  void DrawWaypoints(Canvas &canvas) noexcept;
  void DrawTask(Canvas &canvas) noexcept;
  void DrawTraffic(Canvas &canvas) noexcept;

protected:
  /* virtual methods from class Window */
  void OnCreate() override;
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;

  void OnPaintBuffer(Canvas& canvas) noexcept override;
};

