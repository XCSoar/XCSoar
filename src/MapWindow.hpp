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

#include "MapWindowProjection.hpp"
#include "MapWindowTimer.hpp"
#include "Thread/Trigger.hpp"
#include "Thread/Mutex.hpp"
#include "Screen/PaintWindow.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/LabelBlock.hpp"
#include "MapWindowBlackboard.hpp"
#include "PeriodClock.hpp"
#include "NMEA/Derived.hpp"

#include <vector>

typedef struct _THERMAL_SOURCE_VIEW
{
  POINT Screen;
  bool Visible;
} THERMAL_SOURCE_VIEW;

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
class TerrainRenderer;
class Marks;
class GaugeCDI;
class Waypoints;
class Waypoint;
class AirspaceClientUI;
class TaskClientUI;
class GlidePolar;
class ContainerWindow;

class MapWindow : public PaintWindow,
  public MapWindowProjection,
  public MapWindowBlackboard,
  public MapWindowTimer
{
  PeriodClock mouse_down_clock;

  Waypoints *way_points;
  TopologyStore *topology;
  RasterTerrain *terrain;
  RasterWeather *weather;

  bool topology_dirty, terrain_dirty, weather_dirty;
  unsigned idle_robin;

  TerrainRenderer *terrain_renderer;
  AirspaceClientUI *m_airspace;
  TaskClientUI *task;

  Marks *marks;

  GaugeCDI *cdi;

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

  void set(ContainerWindow &parent,
           const RECT _MapRectSmall, const RECT _MapRectBig);

  void set_way_points(Waypoints *_way_points) {
    way_points = _way_points;
  }

  void set_task(TaskClientUI *_task) {
    task = _task;
  }

  void set_airspaces(AirspaceClientUI *_airspace) {
    m_airspace = _airspace;
  }

  void set_topology(TopologyStore *_topology);
  void set_terrain(RasterTerrain *_terrain);
  void set_weather(RasterWeather *_weather);

  void set_marks(Marks *_marks) {
    marks = _marks;
  }

  // used by dlgTarget
  bool TargetDragged(double *longitude, double *latitude);

  // use at startup
  void SetMapRect(RECT rc) {
    move(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    SetRect(&MapRect, 0, 0, rc.right - rc.left, rc.bottom - rc.top);
  }

  void ReadBlackboard(const NMEA_INFO &nmea_info,
                      const DERIVED_INFO &derived_info,
                      const SETTINGS_COMPUTER &settings_computer,
                      const SETTINGS_MAP &settings_map);

  void UpdateProjection();

  const MapWindowProjection &MapProjection() const {
    return *this;
  };

private:

  void DrawThreadLoop ();
  void DrawThreadInitialise (void);
  Mutex    mutexBuffer;

  void ApplyScreenSize();

  // display management
  void          RefreshMap();
  void          SwitchZoomClimb(void);

  // state/localcopy/local data
  GEOPOINT      TargetDrag_Location;
  int           TargetDrag_State;

  POINT         Groundline[TERRAIN_ALT_INFO::NUMTERRAINSWEEPS + 1];

  ZoomClimb_t    zoomclimb;

  THERMAL_SOURCE_VIEW ThermalSources[MAX_THERMAL_SOURCES];

  // projection
  bool      BigZoom;

  // interface handlers
  int ProcessVirtualKey(int X, int Y, long keytime, short vkmode);

  // display element functions

  void CalculateScreenPositions(POINT Orig, RECT rc,
                                POINT *Orig_Aircraft);

  void CalculateScreenPositionsGroundline();
  void CalculateScreenPositionsThermalSources();
  void MapWaypointLabelSortAndRender(Canvas &canvas);

  // display renderers
  void DrawAircraft(Canvas &canvas);
  void DrawCrossHairs(Canvas &canvas);
  void DrawBestCruiseTrack(Canvas &canvas);
  void DrawCompass(Canvas &canvas, const RECT rc);
  void DrawHorizon(Canvas &canvas, const RECT rc);
  void DrawWindAtAircraft2(Canvas &canvas, POINT Orig, RECT rc);
  void DrawAirspace(Canvas &canvas, Canvas &buffer);
  void DrawAirspaceIntersections(Canvas &canvas);
  void DrawWaypoints(Canvas &canvas);

  void DrawFlightMode(Canvas &canvas, const RECT rc);
  void DrawGPSStatus(Canvas &canvas, const RECT rc, const GPS_STATE &gps);
  void DrawTrail(Canvas &canvas);
  void DrawTeammate(Canvas &canvas);
  void DrawProjectedTrack(Canvas &canvas);
  void DrawTask(Canvas &canvas, RECT rc, Canvas &buffer);
  void DrawThermalEstimate(Canvas &canvas);

  void DrawMapScale(Canvas &canvas, const RECT rc,
			   const bool ScaleChangeFeedback);
  void DrawMapScale2(Canvas &canvas, const RECT rc);
  void DrawFinalGlide(Canvas &canvas, const RECT rc);
  void DrawThermalBand(Canvas &canvas, const RECT rc);
  void DrawGlideThroughTerrain(Canvas &canvas);
  void DrawTerrainAbove(Canvas &hDC, const RECT rc, Canvas &buffer);
  void DrawCDI();
  void DrawSpotHeights(Canvas &canvas);

  // events

  bool AirspaceDetailsAtPoint(const GEOPOINT &location) const;

  //  void DrawSpeedToFly(HDC hDC, RECT rc);
  void DrawFLARMTraffic(Canvas &canvas);
  double    findMapScaleBarSize(const RECT rc);

  // thread, main functions
  void Render(Canvas &canvas, const RECT rc);

  void UpdateTopologyCache();
  void UpdateTopology(bool force);
  void UpdateTerrain();
  void UpdateWeather();

  void UpdateAll() {
    UpdateTopologyCache();
    while (topology_dirty)
      UpdateTopology(true);
    UpdateTerrain();
    UpdateWeather();
  }

  bool Idle(const bool force=false);

  // graphics vars

  BufferCanvas draw_canvas;
  BufferCanvas buffer_canvas;
  BufferCanvas stencil_canvas;
  BitmapCanvas bitmap_canvas;

  LabelBlock label_block;
public:
  bool checkLabelBlock(RECT rc);
  LabelBlock *getLabelBlock() {
    return &label_block;
  }

  void draw_bitmap(Canvas &canvas, const Bitmap &bitmap,
		   const int x, const int y,
		   const unsigned src_x_offset,
		   const unsigned src_y_offset,
		   const unsigned src_width,
		   const unsigned src_height,
		   bool centered=true);

  void draw_masked_bitmap(Canvas &canvas, const Bitmap &bitmap,
			  const int x, const int y,
			  const unsigned src_width,
			  const unsigned src_height,
			  bool centered=true);

  bool draw_masked_bitmap_if_visible(Canvas &canvas,
				     Bitmap &bitmap,
				     const GEOPOINT &loc,
				     unsigned width,
				     unsigned height,
				     POINT *sc=NULL);

protected:
  virtual bool on_create();
  virtual bool on_destroy();
  virtual bool on_resize(unsigned width, unsigned height);
  virtual bool on_mouse_double(int x, int y);
  virtual bool on_mouse_move(int x, int y, unsigned keys);
  virtual bool on_mouse_down(int x, int y);
  virtual bool on_mouse_up(int x, int y);
  virtual bool on_mouse_wheel(int delta);

#if defined(GNAV) || defined(PNA)
  virtual bool on_key_down(unsigned key_code);
#else
  virtual bool on_key_up(unsigned key_code);
#endif

  virtual void on_paint(Canvas& canvas);
  virtual bool on_setfocus();

private:

  /**
   * This (non-virtual, non-inherited) method gets called by either
   * on_key_down() (Altair and PNAs) or on_key_up() (all other
   * platforms).
   *
   * Some PDAs like iPAQ hx4700 send 0xca..0xcd in WM_KEYDOWN, but
   * 0xc0..0xc4 (VK_APP1..4) in WM_KEYUP.  We prefer the VK_APP codes.
   */
  bool on_key_press(unsigned key_code);

  GlidePolar get_glide_polar() const;

  void RenderStart(Canvas &canvas, const RECT rc);
  void RenderBackground(Canvas &canvas, const RECT rc);
  void RenderMapLayer(Canvas &canvas, const RECT rc);
  void RenderAreas(Canvas &canvas, const RECT rc);
  void RenderTrail(Canvas &canvas, const RECT rc);
  void RenderTaskElements(Canvas &canvas, const RECT rc);
  void RenderGlide(Canvas &canvas, const RECT rc);
  void RenderAirborne(Canvas &canvas, const RECT rc);
  void RenderSymbology_upper(Canvas &canvas, const RECT rc);
  void RenderSymbology_lower(Canvas &canvas, const RECT rc);

  std::vector<GEOPOINT> m_airspace_intersections;

  friend class DrawThread;
};

#endif
