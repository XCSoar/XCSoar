/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
#include "DisplayMode.hpp"

struct Look;
class Logger;

struct ZoomClimb_t
{
  fixed CruiseScale;
  fixed ClimbScale;
  bool last_isclimb;

  ZoomClimb_t();
};


class OffsetHistory {
  static const unsigned int historySize = 30;
  unsigned int pos;
  RasterPoint offsets[historySize];

  friend class GlueMapWindow;

protected:
  OffsetHistory() : pos(0) { reset(); }
  void reset();
  void add(PixelScalar x, PixelScalar y);
  void add(const RasterPoint &p) { add(p.x, p.y); }
  RasterPoint average() const;

  static const RasterPoint zeroPoint;
};


class GlueMapWindow : public MapWindow {
  const Logger *logger;

  unsigned idle_robin;

  PeriodClock mouse_down_clock;

  enum DragMode {
    DRAG_NONE,
    DRAG_PAN,
    DRAG_GESTURE,
    DRAG_SIMULATOR,
  } drag_mode;

  GeoPoint drag_start_geopoint;
  RasterPoint drag_start, drag_last;
  GestureManager gestures;
  bool ignore_single_click;

  /** if mouse pan drag has moved over ~10 pixels */
  bool dragOverMinDist;

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
   * MapWindowBlackboard::ReadSettingsMap() before the next frame.
   */
  SETTINGS_MAP next_settings_map;

  /**
   * The new glide computer settings.  It is passed to
   * MapWindowBlackboard::ReadSettingsComputer() before the next
   * frame.
   */
  SETTINGS_COMPUTER next_settings_computer;
#endif

  ThermalBandRenderer thermal_band_renderer;
  FinalGlideBarRenderer final_glide_bar_renderer;

  timer_t map_item_timer;

public:
  GlueMapWindow(const Look &look);

  void SetLogger(Logger *_logger) {
    logger = _logger;
  }

  void SetSettingsMap(const SETTINGS_MAP &new_value);
  void SetSettingsComputer(const SETTINGS_COMPUTER &new_value);

  /**
   * Update the blackboard from DeviceBlackboard and
   * InterfaceBlackboard.
   */
  void ExchangeBlackboard();

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

private:
  bool ShowMapItems(const GeoPoint &location);

protected:
  // events
  virtual bool on_mouse_double(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_move(PixelScalar x, PixelScalar y, unsigned keys);
  virtual bool on_mouse_down(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_up(PixelScalar x, PixelScalar y);
  virtual bool on_mouse_wheel(PixelScalar x, PixelScalar y, int delta);

  /**
   * This event handler gets called when a gesture has
   * been painted by the user
   * @param gesture The gesture string (e.g. "ULR")
   * @return True if the gesture was handled by the
   * event handler, False otherwise
   */
  bool on_mouse_gesture(const TCHAR* gesture);

  virtual bool on_key_down(unsigned key_code);

  virtual bool on_setfocus();
  virtual bool on_cancel_mode();
  virtual void on_paint(Canvas &canvas);
  virtual void on_paint_buffer(Canvas& canvas);
  bool on_timer(timer_t id);

private:
  void DrawMapScale(Canvas &canvas, const PixelRect &rc,
                    const MapWindowProjection &projection) const;
  void DrawFlightMode(Canvas &canvas, const PixelRect &rc) const;
  void DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                     const NMEAInfo &info) const;
  void DrawCrossHairs(Canvas &canvas) const;
  void DrawThermalBand(Canvas &canvas, const PixelRect &rc) const;
  void DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const;
  void DrawStallRatio(Canvas &canvas, const PixelRect &rc) const;
  virtual void DrawThermalEstimate(Canvas &canvas) const;
  virtual void RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos) const;

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
