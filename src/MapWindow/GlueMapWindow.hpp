/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_GLUE_MAP_WINDOW_HPP
#define XCSOAR_GLUE_MAP_WINDOW_HPP

#include "MapWindow.hpp"
#include "Time/PeriodClock.hpp"
#include "UIUtil/TrackingGestureManager.hpp"
#include "UIUtil/KineticManager.hpp"
#include "Renderer/ThermalBandRenderer.hpp"
#include "Renderer/FinalGlideBarRenderer.hpp"
#include "Renderer/VarioBarRenderer.hpp"
#include "Screen/Timer.hpp"
#include "Screen/Features.hpp"

#include <array>

struct Look;
struct GestureLook;
class Logger;
class TopographyThread;

class OffsetHistory
{
  unsigned int pos;
  std::array<RasterPoint, 30> offsets;

public:
  OffsetHistory():pos(0) {
    Reset();
  }

  void Reset();
  void Add(RasterPoint p);
  RasterPoint GetAverage() const;
};


class GlueMapWindow : public MapWindow {
  enum class Command {
    INVALIDATE,
  };

  TopographyThread *topography_thread;

  const Logger *logger;

#ifdef ENABLE_OPENGL
  /**
   * A timer that triggers a redraw periodically until all data files
   * (terrain, topography, ...) have been loaded / updated.  This is
   * necessary if there is no valid GPS input, and no other reason to
   * redraw is present.  This timer will cease automatically once all
   * data is loaded, i.e. Idle() returned false.
   */
  WindowTimer data_timer;
#endif

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
  } drag_mode;

  GeoPoint drag_start_geopoint;
  RasterPoint drag_start;
  TrackingGestureManager gestures;
  bool ignore_single_click;

  /**
   * Skip the next Idle() call?  This is set to true when a new frame
   * shall be rendered quickly without I/O delay, e.g. to display the
   * first frame quickly.
   */
  bool skip_idle;

#ifdef ENABLE_OPENGL
  KineticManager kinetic_x, kinetic_y;
  WindowTimer kinetic_timer;
#endif

  /** flag to indicate if the MapItemList should be shown on mouse up */
  bool arm_mapitem_list;

  /**
   * The projection which was active when dragging started.
   */
  Projection drag_projection;

  DisplayMode last_display_mode;

  OffsetHistory offset_history;

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

  WindowTimer map_item_timer;

public:
  GlueMapWindow(const Look &look);
  virtual ~GlueMapWindow();

  void SetTopography(TopographyStore *_topography);

  void SetLogger(Logger *_logger) {
    logger = _logger;
  }

  void SetMapSettings(const MapSettings &new_value);
  void SetComputerSettings(const ComputerSettings &new_value);
  void SetUIState(const UIState &new_value);

  /**
   * Update the blackboard from DeviceBlackboard and
   * InterfaceBlackboard.
   */
  void ExchangeBlackboard();

  /**
   * Suspend threads that are owned by this object.
   */
  void SuspendThreads();

  /**
   * Resumt threads that are owned by this object.
   */
  void ResumeThreads();

  /**
   * Trigger a full redraw of the map.
   */
  void FullRedraw();

  void QuickRedraw();

  bool Idle();

  void Create(ContainerWindow &parent, const PixelRect &rc);

  void SetPan(bool enable);
  void TogglePan();
  void PanTo(const GeoPoint &location);

  bool ShowMapItems(const GeoPoint &location,
                    bool show_empty_message = true) const;

protected:
  /* virtual methods from class MapWindow */
  virtual void Render(Canvas &canvas, const PixelRect &rc) override;
  virtual void DrawThermalEstimate(Canvas &canvas) const override;
  virtual void RenderTrail(Canvas &canvas,
                           const RasterPoint aircraft_pos) override;
  virtual void RenderTrackBearing(Canvas &canvas,
                                  const RasterPoint aircraft_pos) override;

  /* virtual methods from class Window */
  virtual void OnDestroy() override;
  virtual bool OnMouseDouble(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys) override;
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y) override;
  virtual bool OnMouseWheel(PixelScalar x, PixelScalar y, int delta) override;

#ifdef HAVE_MULTI_TOUCH
  virtual bool OnMultiTouchDown() override;
#endif

  virtual bool OnKeyDown(unsigned key_code) override;
  virtual void OnCancelMode() override;
  virtual void OnPaint(Canvas &canvas) override;
  virtual void OnPaintBuffer(Canvas& canvas) override;
  virtual bool OnTimer(WindowTimer &timer) override;
  bool OnUser(unsigned id) override;

  /**
   * This event handler gets called when a gesture has
   * been painted by the user
   * @param gesture The gesture string (e.g. "ULR")
   * @return True if the gesture was handled by the
   * event handler, False otherwise
   */
  bool OnMouseGesture(const TCHAR* gesture);

private:
  void DrawGesture(Canvas &canvas) const;
  void DrawMapScale(Canvas &canvas, const PixelRect &rc,
                    const MapWindowProjection &projection) const;
  void DrawFlightMode(Canvas &canvas, const PixelRect &rc) const;
  void DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                     const NMEAInfo &info) const;
  void DrawCrossHairs(Canvas &canvas) const;
  void DrawPanInfo(Canvas &canvas) const;
  void DrawThermalBand(Canvas &canvas, const PixelRect &rc) const;
  void DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const;
  void DrawVario(Canvas &canvas, const PixelRect &rc) const;
  void DrawStallRatio(Canvas &canvas, const PixelRect &rc) const;

  void SwitchZoomClimb();

  void SaveDisplayModeScales();

  /**
   * The attribute visible_projection has been edited.
   */
  void OnProjectionModified() {}

  /**
   * Invoke WindowProjection::UpdateScreenBounds() and trigger updates
   * of data file caches for the new bounds (e.g. topography).
   */
  void UpdateScreenBounds();

  void UpdateScreenAngle();
  void UpdateProjection();

public:
  void SetLocation(const GeoPoint location);

  /**
   * Update the visible_projection location, but only if the new
   * location is sufficiently distant from the current one.  This
   * shall avoid unnecessary map jiggling.  This is a great
   * improvement for E Ink displays to reduce flickering.
   */
  void SetLocationLazy(const GeoPoint location);

  void UpdateMapScale();

  /**
   * Restore the map scale from MapSettings::cruise_scale or
   * MapSettings::circling_scale.
   */
  void RestoreMapScale();

  void UpdateDisplayMode();
  void SetMapScale(fixed scale);

protected:
  DisplayMode GetDisplayMode() const {
    return GetUIState().display_mode;
  }

  bool InCirclingMode() const {
    return GetUIState().display_mode == DisplayMode::CIRCLING;
  }
};

#endif
