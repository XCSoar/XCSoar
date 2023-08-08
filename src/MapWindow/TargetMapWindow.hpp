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

#ifndef ENABLE_OPENGL
#include "ui/canvas/BufferCanvas.hpp"
#endif

struct WaypointLook;
struct TaskLook;
struct AircraftLook;
struct TopographyLook;
struct OverlayLook;
class ContainerWindow;
class TopographyStore;
class TopographyRenderer;
class Waypoints;
class Airspaces;
class ProtectedTaskManager;
class GlideComputer;

class TargetMapWindow : public BufferWindow {
  const TaskLook &task_look;
  const AircraftLook &aircraft_look;
  const TopographyLook &topography_look;
  const OverlayLook &overlay_look;

#ifndef ENABLE_OPENGL
  // graphics vars

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

  unsigned target_index;

  enum DragMode {
    DRAG_NONE,

    /**
     * User has pressed the finger near the target, but this initial
     * position is outside the OZ.  This mode will be changed to
     * #DRAG_TARGET as soon as the finger moves to a valid location
     * inside the OZ.
     */
    DRAG_TARGET_OUTSIDE,

    /**
     * Target is being dragged (it has been moved already).  Releasing
     * the finger will commit the change.
     */
    DRAG_TARGET,

    /**
     * User has pressed the finger on the observation zone; if he
     * releases and hasn't moved the finger, the target will be moved
     * here (without having to drag the target explicitly).
     */
    DRAG_OZ,
  } drag_mode;

  PixelPoint drag_start, drag_last;

public:
  TargetMapWindow(const WaypointLook &waypoint_look,
                  const AirspaceLook &airspace_look,
                  const TrailLook &trail_look,
                  const TaskLook &task_look,
                  const AircraftLook &aircraft_look,
                  const TopographyLook &topography_look,
                  const OverlayLook &overlay_look) noexcept;
  virtual ~TargetMapWindow() noexcept;

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

  void SetTarget(unsigned index) noexcept;

private:
  /**
   * Renders the terrain background
   * @param canvas The drawing canvas
   */
  void RenderTerrain(Canvas &canvas) noexcept;

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

  /**
   * Renders the airspace
   * @param canvas The drawing canvas
   */
  void RenderAirspace(Canvas &canvas) noexcept;

  void RenderTrail(Canvas &canvas) noexcept;

  void DrawWaypoints(Canvas &canvas) noexcept;

  void DrawTask(Canvas &canvas) noexcept;

private:
  /**
   * If PanTarget, paints target during drag
   * Used by dlgTarget
   *
   * @param drag_last location of target
   * @param canvas
   */
  void TargetPaintDrag(Canvas &canvas, PixelPoint last_drag) noexcept;

  /**
   * If PanTarget, tests if target is clicked
   * Used by dlgTarget
   *
   * @param drag_last location of click
   *
   * @return true if click is near target
   */
  [[gnu::pure]]
  bool isClickOnTarget(PixelPoint drag_last) const noexcept;

  /**
   * If PanTarget, tests if drag destination
   * is in OZ of target being edited
   * Used by dlgTarget
   *
   * @param x mouse_up location
   * @param y mouse_up location
   *
   * @return true if location is in OZ
   */
  [[gnu::pure]]
  bool isInSector(PixelPoint p) const noexcept;

  /**
   * If PanTarget, updates task with new target
   * Used by dlgTarget
   *
   * @param x mouse_up location
   * @param y mouse_up location
   *
   * @return true if successful
   */
  bool TargetDragged(PixelPoint p) noexcept;

protected:
  virtual void OnTaskModified() noexcept;

protected:
  /* virtual methods from class Window */
  void OnCreate() override;
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;

  void OnPaintBuffer(Canvas& canvas) noexcept override;
  void OnPaint(Canvas& canvas) noexcept override;

  void OnCancelMode() noexcept override;

  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
};
