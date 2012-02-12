/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "PeriodClock.hpp"
#include "GestureManager.hpp"
#include "Renderer/ThermalBandRenderer.hpp"
#include "Renderer/FinalGlideBarRenderer.hpp"
#include "Screen/Timer.hpp"
#include "Screen/Features.hpp"
#include "DisplayMode.hpp"

#include <array>

struct Look;
class Logger;
class SingleWindow;

struct ZoomClimb_t
{
  fixed CruiseScale;
  fixed ClimbScale;
  bool last_isclimb;

  ZoomClimb_t();
};


class OffsetHistory {
  unsigned int pos;
  std::array<RasterPoint, 30> offsets;

  friend class GlueMapWindow;

protected:
  OffsetHistory() : pos(0) { reset(); }
  void reset();
  void add(RasterPoint p);
  RasterPoint average() const;
};


class GlueMapWindow : public MapWindow {
  const Logger *logger;

  unsigned idle_robin;

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
  RasterPoint drag_start, drag_last;
  GestureManager gestures;
  bool ignore_single_click;

  /** flag to indicate if the MapItemList should be shown on mouse up */
  bool arm_mapitem_list;

  ZoomClimb_t zoomclimb;

  /**
   * The projection which was active when dragging started.
   */
  Projection drag_projection;

  enum DisplayMode DisplayMode;

  OffsetHistory offsetHistory;

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
#endif

  ThermalBandRenderer thermal_band_renderer;
  FinalGlideBarRenderer final_glide_bar_renderer;

  WindowTimer map_item_timer;

public:
  GlueMapWindow(const Look &look);

  void SetLogger(Logger *_logger) {
    logger = _logger;
  }

  void SetMapSettings(const MapSettings &new_value);
  void SetComputerSettings(const ComputerSettings &new_value);

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

  virtual void Render(Canvas &canvas, const PixelRect &rc);

  virtual void set(ContainerWindow &parent, const PixelRect &rc);

  void SetPan(bool enable);
  void TogglePan();
  void PanTo(const GeoPoint &location);

  /**
   * If point is in any active OZ of the current task,
   * it returns the index of that turnpoint.
   *
   * Used to popup the dlgTarget
   *
   * @param gp location where click started
   *
   * @return -1 if not in any sector, else tp index
   *        if tp index >= task's ActiveIndex
   */
  int isInAnyActiveSector(const GeoPoint &gp);

  bool ShowMapItems(const GeoPoint &location,
                    bool show_empty_message = true) const;

protected:
  // events
  virtual bool OnMouseDouble(PixelScalar x, PixelScalar y);
  virtual bool OnMouseMove(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool OnMouseDown(PixelScalar x, PixelScalar y);
  virtual bool OnMouseUp(PixelScalar x, PixelScalar y);
  virtual bool OnMouseWheel(PixelScalar x, PixelScalar y, int delta);

#ifdef HAVE_MULTI_TOUCH
  virtual bool OnMultiTouchDown();
#endif

  /**
   * This event handler gets called when a gesture has
   * been painted by the user
   * @param gesture The gesture string (e.g. "ULR")
   * @return True if the gesture was handled by the
   * event handler, False otherwise
   */
  bool on_mouse_gesture(const TCHAR* gesture);

  virtual bool OnKeyDown(unsigned key_code);
  virtual bool OnCancelMode();
  virtual void OnPaint(Canvas &canvas);
  virtual void OnPaintBuffer(Canvas& canvas);
  bool OnTimer(WindowTimer &timer);

private:
  void DrawMapScale(Canvas &canvas, const PixelRect &rc,
                    const MapWindowProjection &projection) const;
  void DrawFlightMode(Canvas &canvas, const PixelRect &rc) const;
  void DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                     const NMEAInfo &info) const;
  void DrawCrossHairs(Canvas &canvas) const;
  void DrawPanInfo(Canvas &canvas) const;
  void DrawThermalBand(Canvas &canvas, const PixelRect &rc) const;
  void DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const;
  void DrawStallRatio(Canvas &canvas, const PixelRect &rc) const;
  virtual void DrawThermalEstimate(Canvas &canvas) const;
  virtual void RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos);

  void SwitchZoomClimb();

  void LoadDisplayModeScales();
  void SaveDisplayModeScales();

  void UpdateScreenAngle();
  void UpdateProjection();

  /**
   * Update the visible_projection location, but only if the new
   * location is sufficiently distant from the current one.  This
   * shall avoid unnecessary map jiggling.  This is a great
   * improvement for E Ink displays to reduce flickering.
   */
  void SetLocationLazy(const GeoPoint location);

public:
  void UpdateMapScale();
  void UpdateDisplayMode();
  void SetMapScale(const fixed x);

  enum DisplayMode GetDisplayMode() const {
    return DisplayMode;
  }
};

#endif
