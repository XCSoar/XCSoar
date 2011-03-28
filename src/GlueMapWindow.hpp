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

struct ZoomClimb_t
{
  fixed CruiseScale;
  fixed ClimbScale;
  bool last_isclimb;
  bool last_targetpan;

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
  void add(int x, int y);
  void add(const RasterPoint &p) { add(p.x, p.y); }
  RasterPoint average() const;

  static const RasterPoint zeroPoint;
};


class GlueMapWindow : public MapWindow {
  unsigned idle_robin;

  PeriodClock mouse_down_clock;

  enum drag_mode {
    DRAG_NONE,
    DRAG_PAN,
    DRAG_GESTURE,
    DRAG_SIMULATOR,
    DRAG_TARGET,
  } drag_mode;

  GeoPoint drag_start_geopoint;
  RasterPoint drag_start, drag_last;
  GestureManager gestures;
  bool ignore_single_click;
  bool dragOverMinDist; /* <// if mouse pan drag has moved over ~10 pixels */

  ZoomClimb_t zoomclimb;

  /**
   * The projection which was active when dragging started.
   */
  Projection drag_projection;

  DisplayMode_t DisplayMode;

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
#endif

public:
  GlueMapWindow();

  void SetSettingsMap(const SETTINGS_MAP &new_value);

  /**
   * Update the blackboard from DeviceBlackboard and
   * InterfaceBlackboard.
   */
  void ExchangeBlackboard();

  /**
   * Trigger a full redraw of the map.
   */
  void FullRedraw();

  void QuickRedraw(const SETTINGS_MAP &_settings_map);

  bool Idle();

  virtual void Render(Canvas &canvas, const PixelRect &rc);

  virtual void set(ContainerWindow &parent, const PixelRect &rc);

  /**
   * If PanTarget, paints target during drag
   * Used by dlgTarget
   *
   * @param drag_last location of target
   * @param canvas
   */
  void TargetPaintDrag(Canvas &canvas, const RasterPoint last_drag);

  /**
   * If PanTarget, tests if target is clicked
   * Used by dlgTarget
   *
   * @param drag_last location of click
   *
   * @return true if click is near target
   */
  bool isClickOnTarget(const RasterPoint drag_last);

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
  bool isInSector(const int x, const int y);

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

  /**
   * If PanTarget, updates task with new target
   * Used by dlgTarget
   *
   * @param x mouse_up location
   * @param y mouse_up location
   *
   * @return true if successful
   */
  bool TargetDragged(const int x, const int y);

private:
  bool AirspaceDetailsAtPoint(const GeoPoint &location);

protected:
  // events
  virtual bool on_mouse_double(int x, int y);
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_mouse_wheel(int delta);
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

private:
  void DrawMapScale(Canvas &canvas, const PixelRect &rc,
                    const MapWindowProjection &projection) const;
  void DrawFlightMode(Canvas &canvas, const PixelRect &rc) const;
  void DrawGPSStatus(Canvas &canvas, const PixelRect &rc,
                     const NMEA_INFO &info) const;
  void DrawCrossHairs(Canvas &canvas) const;
  void DrawFinalGlide(Canvas &canvas, const PixelRect &rc) const;
  void DrawHorizon(Canvas &canvas, const PixelRect &rc) const;
  virtual void DrawThermalEstimate(Canvas &canvas) const;
  virtual void RenderTrail(Canvas &canvas, const RasterPoint aircraft_pos) const;

  void SwitchZoomClimb();

  void LoadDisplayModeScales();
  void SaveDisplayModeScales();

  void UpdateScreenAngle();
  void UpdateProjection();

public:
  void UpdateMapScale();
  void UpdateDisplayMode();
  void SetMapScale(const fixed x);

  DisplayMode_t GetDisplayMode() const {
    return DisplayMode;
  }
};

#endif
