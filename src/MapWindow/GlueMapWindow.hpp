// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "MapWindow.hpp"
#include "time/PeriodClock.hpp"
#include "UIUtil/TrackingGestureManager.hpp"
#include "UIUtil/KineticManager.hpp"
#include "Renderer/ThermalBandRenderer.hpp"
#include "Renderer/FinalGlideBarRenderer.hpp"
#include "Renderer/VarioBarRenderer.hpp"
#include "ui/event/Timer.hpp"
#include "ui/event/Notify.hpp"
#include "ui/window/Features.hpp"

#ifdef ENABLE_OPENGL
#include "ui/event/PeriodicTimer.hpp"
#endif

#include <array>

struct Look;
struct GestureLook;
class TopographyThread;
class TerrainThread;

class OffsetHistory
{
  unsigned int pos = 0;
  std::array<PixelPoint, 30> offsets;

public:
  OffsetHistory() noexcept {
    Reset();
  }

  void Reset() noexcept;
  void Add(PixelPoint p) noexcept;
  PixelPoint GetAverage() const noexcept;
};


class GlueMapWindow : public MapWindow {
  TopographyThread *topography_thread = nullptr;

  TerrainThread *terrain_thread = nullptr;

  PeriodClock mouse_down_clock;

  enum DragMode {
    DRAG_NONE,

#ifdef HAVE_MULTI_TOUCH
    /**
     * Dragging the map with two fingers; enters the "real" pan mode
     * as soon as the user releases the finger press.
     */
    DRAG_MULTI_TOUCH_PAN,
#endif

    DRAG_PAN,
    DRAG_GESTURE,
    DRAG_SIMULATOR,
  } drag_mode = DRAG_NONE;

  GeoPoint drag_start_geopoint;
  PixelPoint drag_start;
  TrackingGestureManager gestures;
  bool ignore_single_click = false;

#ifdef ENABLE_OPENGL
  KineticManager kinetic_x{std::chrono::milliseconds{700}};
  KineticManager kinetic_y{std::chrono::milliseconds{700}};
  UI::PeriodicTimer kinetic_timer{[this]{ OnKineticTimer(); }};
#endif

  /** flag to indicate if the MapItemList should be shown on mouse up */
  bool arm_mapitem_list = false;

  /**
   * The projection which was active when dragging started.
   */
  Projection drag_projection;

  DisplayMode last_display_mode = DisplayMode::NONE;

  OffsetHistory offset_history;

  /*
   * Area of the map where no HUD items should be drawn
   */
  unsigned int bottom_margin = 0;

#ifndef ENABLE_OPENGL
  /**
   * This mutex protects the attributes that are read by the
   * DrawThread but written by another thread.
   */
  Mutex next_mutex;

  /**
   * The new map settings.  It is passed to
   * MapWindowBlackboard::ReadMapSettings() before the next frame.
   */
  MapSettings next_settings_map;

  /**
   * The new glide computer settings.  It is passed to
   * MapWindowBlackboard::ReadGetComputerSettings() before the next
   * frame.
   */
  ComputerSettings next_settings_computer;

  UIState next_ui_state;
#endif

  ThermalBandRenderer thermal_band_renderer;
  FinalGlideBarRenderer final_glide_bar_renderer;
  VarioBarRenderer vario_bar_renderer;

  const GestureLook &gesture_look;

  UI::Timer map_item_timer{[this]{ OnMapItemTimer(); }};

  UI::Notify redraw_notify{[this]{ PartialRedraw(); }};

public:
  GlueMapWindow(const Look &look) noexcept;
  virtual ~GlueMapWindow() noexcept;

  void SetTopography(TopographyStore *_topography) noexcept;
  void SetTerrain(RasterTerrain *_terrain) noexcept;

  void SetMapSettings(const MapSettings &new_value) noexcept;
  void SetComputerSettings(const ComputerSettings &new_value) noexcept;
  void SetUIState(const UIState &new_value) noexcept;

  /**
   * Sets a relative margin at the bottom of the screen where no HUD
   * elements should be drawn.
   */
  void SetBottomMargin(unsigned margin) noexcept;

  void SetBottomMarginFactor(unsigned margin_factor) noexcept;

  /**
   * Update the blackboard from DeviceBlackboard and
   * InterfaceBlackboard.
   */
  void ExchangeBlackboard() noexcept;

  /**
   * Suspend threads that are owned by this object.
   */
  void SuspendThreads() noexcept;

  /**
   * Resumt threads that are owned by this object.
   */
  void ResumeThreads() noexcept;

  /**
   * Trigger a full redraw of the map.
   */
  void FullRedraw() noexcept;
  void PartialRedraw() noexcept;

  void QuickRedraw() noexcept;

  /**
   * Trigger a deferred redraw.  It will occur in the main thread
   * after all other events have been handled.
   *
   * This method is thread-safe.
   */
  void InjectRedraw() noexcept {
    redraw_notify.SendNotification();
  }

  /**
   * Trigger a deferred redraw.  It will occur in the main thread
   * after all other events have been handled.
   */
  void DeferRedraw() noexcept {
#ifdef ENABLE_OPENGL
    /* with OpenGL, redraws are synchronous (no DrawThread), but
       Invalidate() defers this until the whole screen is redrawn */
    Invalidate();
#else
    /* without OpenGL, we have a DrawThread, and the redraw_notify
       will defer the DrawThread wakeup to merge adjacent calls to
       this method */
    InjectRedraw();
#endif
  }

  void SetPan(bool enable) noexcept;
  void TogglePan() noexcept;
  void PanTo(const GeoPoint &location) noexcept;

  bool ShowMapItems(const GeoPoint &location, bool show_empty_message = true,
                    bool pointer_in_use = true) const noexcept;

protected:
  /* virtual methods from class MapWindow */
  void Render(Canvas &canvas, const PixelRect &rc) noexcept override;
  void DrawThermalEstimate(Canvas &canvas) const noexcept override;
  void RenderTrail(Canvas &canvas,
                   const PixelPoint aircraft_pos) noexcept override;
  void RenderTrackBearing(Canvas &canvas,
                          const PixelPoint aircraft_pos) noexcept override;

  /* virtual methods from class Window */
  void OnCreate() override;
  void OnDestroy() noexcept override;
  bool OnMouseDouble(PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p, unsigned keys) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp(PixelPoint p) noexcept override;
  bool OnMouseWheel(PixelPoint p, int delta) noexcept override;

#ifdef HAVE_MULTI_TOUCH
  bool OnMultiTouchDown() noexcept override;
#endif

  bool OnKeyDown(unsigned key_code) noexcept override;
  void OnCancelMode() noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  void OnPaintBuffer(Canvas& canvas) noexcept override;

  /**
   * This event handler gets called when a gesture has
   * been painted by the user
   * @param gesture The gesture string (e.g. "ULR")
   * @return True if the gesture was handled by the
   * event handler, False otherwise
   */
  bool OnMouseGesture(const char* gesture) noexcept;

private:
  void DrawGesture(Canvas &canvas) const noexcept;
  void DrawMapScale(Canvas &canvas, const PixelRect &rc,
                    const MapWindowProjection &projection) const noexcept;
  void DrawFlightMode(Canvas &canvas, const PixelRect &rc) const noexcept;
  void DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                     const NMEAInfo &info) const noexcept;
  void DrawCrossHairs(Canvas &canvas) const noexcept;
  void DrawPanInfo(Canvas &canvas) const noexcept;
  void DrawThermalBand(Canvas &canvas, const PixelRect &rc) const noexcept;
  void DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const noexcept;
  void DrawVario(Canvas &canvas, const PixelRect &rc) const noexcept;
  void DrawStallRatio(Canvas &canvas, const PixelRect &rc) const noexcept;

  void SwitchZoomClimb() noexcept;

  void SaveDisplayModeScales() noexcept;

  /**
   * The attribute visible_projection has been edited.
   */
  void OnProjectionModified() noexcept {}

  /**
   * Invoke WindowProjection::UpdateScreenBounds() and trigger updates
   * of data file caches for the new bounds (e.g. topography).
   */
  void UpdateScreenBounds() noexcept;

  void UpdateScreenAngle() noexcept;
  void UpdateProjection() noexcept;

public:
  void SetLocation(const GeoPoint location) noexcept;

  /**
   * Update the visible_projection location, but only if the new
   * location is sufficiently distant from the current one.  This
   * shall avoid unnecessary map jiggling.  This is a great
   * improvement for E Ink displays to reduce flickering.
   */
  void SetLocationLazy(const GeoPoint location) noexcept;

  void UpdateMapScale() noexcept;

  /**
   * Restore the map scale from MapSettings::cruise_scale or
   * MapSettings::circling_scale.
   */
  void RestoreMapScale() noexcept;

  void UpdateDisplayMode() noexcept;
  void SetMapScale(double scale) noexcept;

protected:
  DisplayMode GetDisplayMode() const noexcept {
    return GetUIState().display_mode;
  }

  bool InCirclingMode() const noexcept {
    return GetUIState().display_mode == DisplayMode::CIRCLING;
  }

private:
  void OnMapItemTimer() noexcept;

#ifdef ENABLE_OPENGL
  void OnKineticTimer() noexcept;
#endif
};
