/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_MAP_WINDOW_HPP
#define XCSOAR_MAP_WINDOW_HPP

#include "Util/StaticArray.hpp"
#include "MapWindowProjection.hpp"
#include "MapWindowTimer.hpp"
#include "Screen/DoubleBufferWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/LabelBlock.hpp"
#include "MapWindowBlackboard.hpp"
#include "NMEA/Derived.hpp"
#include "BackgroundDrawHelper.hpp"
#include "WayPoint/WayPointRenderer.hpp"
#include "Compiler.h"

struct THERMAL_SOURCE_VIEW
{
  POINT Screen;
  bool Visible;
};

struct ZoomClimb_t
{
  fixed CruiseMapScale;
  fixed ClimbMapScale;
  bool last_isclimb;
  bool last_targetpan;
};

class TopologyStore;
class RasterTerrain;
class RasterWeather;
class Marks;
class GaugeCDI;
class Waypoints;
class Waypoint;
class Airspaces;
class ProtectedAirspaceWarningManager;
class ProtectedTaskManager;
class GlidePolar;
class ContainerWindow;
class WayPointLabelList;

class MapWindow : public DoubleBufferWindow,
  public MapWindowBlackboard,
  public MapWindowTimer
{
protected:
  /**
   * The projection as currently visible on the screen.  This object
   * is being edited by the user.
   */
  MapWindowProjection visible_projection;

  /**
   * The projection of the buffer.  This differs from
   * visible_projection only after the projection was modified, until
   * the DrawThread has finished drawing the new projection.
   */
  MapWindowProjection buffer_projection;

  /**
   * The projection of the DrawThread.  This is used to render the new
   * map.  After rendering has completed, this object is copied over
   * #buffer_projection.
   */
  MapWindowProjection render_projection;

  const Waypoints *way_points;
  TopologyStore *topology;

  RasterTerrain *terrain;
  GeoPoint terrain_center;

  RasterWeather *weather;

  BackgroundDrawHelper m_background;
  WayPointRenderer way_point_renderer;

  Airspaces *airspace_database;
  ProtectedAirspaceWarningManager *airspace_warnings;
  ProtectedTaskManager *task;

  Marks *marks;

  GaugeCDI *cdi;

  /**
   * Tracks whether the buffer canvas contains valid data.  We use
   * those attributes to prevent showing invalid data on the map, when
   * the user switches quickly to or from full-screen mode.
   */
  unsigned ui_generation, buffer_generation;

  /**
   * If non-zero, then the buffer will be scaled to the new
   * projection, and this variable is decremented.  This is used while
   * zooming and panning, to give instant visual feedback.
   */
  unsigned scale_buffer;

public:
  MapWindow();
  virtual ~MapWindow();

  static bool register_class(HINSTANCE hInstance);

#ifdef WIN32
  /**
   * Identifies the HWND: if the handle is a MapWindow instance, this
   * function returns true.
   */
  static bool identify(HWND hWnd);
#endif

  void set(ContainerWindow &parent, const RECT &rc);

  void set_way_points(const Waypoints *_way_points) {
    way_points = _way_points;
    way_point_renderer.set_way_points(way_points);
  }

  void set_task(ProtectedTaskManager *_task) {
    task = _task;
  }

  void set_airspaces(Airspaces *_airspace_database,
                     ProtectedAirspaceWarningManager *_airspace_warnings) {
    airspace_database = _airspace_database;
    airspace_warnings = _airspace_warnings;
  }

  void set_topology(TopologyStore *_topology);
  void set_terrain(RasterTerrain *_terrain);
  void set_weather(RasterWeather *_weather);

  void set_marks(Marks *_marks) {
    marks = _marks;
  }


  /**
   * If PanTarget, tests if target is clicked
   * Used by dlgTarget
   *
   * @param drag_last location of click
   *
   * @return true if click is near target
   */
  bool isClickOnTarget(const POINT drag_last);

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
   * If PanTarget, paints target during drag
   * Used by dlgTarget
   *
   * @param drag_last location of target
   * @param canvas
   */
  void TargetPaintDrag(Canvas &canvas, const POINT last_drag);

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

  void ReadBlackboard(const NMEA_INFO &nmea_info,
                      const DERIVED_INFO &derived_info,
                      const SETTINGS_COMPUTER &settings_computer,
                      const SETTINGS_MAP &settings_map);

  void UpdateProjection();

  const MapWindowProjection &VisibleProjection() const {
    return visible_projection;
  }

private:

  void DrawThreadLoop ();
  void DrawThreadInitialise (void);

  void ApplyScreenSize();

  // display management
  void SwitchZoomClimb(void);

  // state/localcopy/local data
  GeoPoint TargetDrag_Location;
  int TargetDrag_State;

  POINT Groundline[TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1];

  ZoomClimb_t zoomclimb;

  THERMAL_SOURCE_VIEW ThermalSources[MAX_THERMAL_SOURCES];

  // display element functions

  void CalculateScreenPositions(const POINT &Orig, const RECT &rc,
                                POINT *Orig_Aircraft);

  void CalculateScreenPositionsGroundline();
  void CalculateScreenPositionsThermalSources();
  void MapWaypointLabelRender(Canvas &canvas, const RECT &rc,
                              const WayPointLabelList &labels);

  // display renderers
  void DrawAircraft(Canvas &canvas) const;

protected:
  void DrawCrossHairs(Canvas &canvas) const;

public:
  void DrawBestCruiseTrack(Canvas &canvas) const;
  void DrawCompass(Canvas &canvas, const RECT &rc) const;
  void DrawHorizon(Canvas &canvas, const RECT &rc) const;
  void DrawWindAtAircraft2(Canvas &canvas, const POINT &Orig,
                           const RECT &rc) const;
  void DrawAirspace(Canvas &canvas, Canvas &buffer);
  void DrawAirspaceIntersections(Canvas &canvas) const;
  void DrawWaypoints(Canvas &canvas, const RECT &rc);

  void DrawFlightMode(Canvas &canvas, const RECT &rc) const;
  void DrawGPSStatus(Canvas &canvas, const RECT &rc,
                     const GPS_STATE &gps) const;
  void DrawTrail(Canvas &canvas) const;
  void DrawTeammate(Canvas &canvas) const;
  void DrawProjectedTrack(Canvas &canvas) const;
  void DrawTask(Canvas &canvas, const RECT &rc, Canvas &buffer);
  void DrawThermalEstimate(Canvas &canvas) const;

  void DrawMapScale(Canvas &canvas, const RECT &rc,
                    const MapWindowProjection &projection) const;
  void DrawMapScale2(Canvas &canvas, const RECT &rc,
                     const MapWindowProjection &projection) const;
  void DrawFinalGlide(Canvas &canvas, const RECT &rc) const;
  void DrawThermalBand(Canvas &canvas, const RECT &rc) const;
  void DrawGlideThroughTerrain(Canvas &canvas) const;
  void DrawTerrainAbove(Canvas &hDC, Canvas &buffer);
  void DrawCDI();

  //  void DrawSpeedToFly(HDC hDC, RECT rc);
  void DrawFLARMTraffic(Canvas &canvas) const;

  gcc_pure
  fixed findMapScaleBarSize(const RECT &rc, const MapWindowProjection &projection) const;

  // thread, main functions
  void Render(Canvas &canvas, const RECT &rc);

protected:
  void UpdateTopology();
  void UpdateTerrain();
  void UpdateWeather();

  void UpdateAll() {
    UpdateTopology();
    UpdateTerrain();
    UpdateWeather();
  }

private:
  // graphics vars

  BufferCanvas buffer_canvas;
  BufferCanvas stencil_canvas;

protected:
  mutable BitmapCanvas bitmap_canvas;

private:
  LabelBlock label_block;

public:
  BitmapCanvas& get_bitmap_canvas() { return bitmap_canvas; };

  LabelBlock &getLabelBlock() {
    return label_block;
  }

protected:
  virtual bool on_create();
  virtual bool on_destroy();
  virtual bool on_resize(unsigned width, unsigned height);

  virtual void on_paint(Canvas& canvas);

private:
  GlidePolar get_glide_polar() const;

  void RenderStart(Canvas &canvas, const RECT &rc);
  void RenderMapLayer(Canvas &canvas);
  void RenderAreas(Canvas &canvas, const RECT &rc);
  void RenderTrail(Canvas &canvas);
  void RenderTaskElements(Canvas &canvas, const RECT &rc);
  void RenderGlide(Canvas &canvas, const RECT &rc);
  void RenderAirborne(Canvas &canvas, const RECT &rc);
  void RenderSymbology_upper(Canvas &canvas, const RECT &rc);
  void RenderSymbology_lower(Canvas &canvas, const RECT &rc);

  StaticArray<GeoPoint,32> m_airspace_intersections;

  friend class DrawThread;
};

#endif
