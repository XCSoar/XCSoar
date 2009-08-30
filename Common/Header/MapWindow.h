/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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
#if !defined(XCSOAR_MAPWINDOW_H)
#define XCSOAR_MAPWINDOW_H

#include "XCSoar.h"
#include "MapWindowProjection.hpp"
#include "Airspace.h"
#include "Trigger.hpp"
#include "Mutex.hpp"
#include "Screen/BufferCanvas.hpp"
#include "Screen/BitmapCanvas.hpp"
#include "Screen/PaintWindow.hpp"

class MapWindowBase {
 public:
  static Trigger  dirtyEvent;
  static void     CloseDrawingThread(void);
  static void     SuspendDrawingThread(void);
  static void     ResumeDrawingThread(void);
  static bool     IsDisplayRunning();
  static void     CreateDrawingThread(void);
  static Mutex    mutexRun;
 protected:
  static DWORD    dwDrawThreadID;
  static HANDLE   hDrawThread;
  static Mutex    mutexStart;
  static bool     window_initialised;
};


class MapWindow
: public PaintWindow, public MapWindowBase,
  public MapWindowProjection {
 public:

  bool register_class(HINSTANCE hInstance, const TCHAR* szWindowClass);

  // inter-process, used only on file change
  static void ForceVisibilityScan() {
    askVisibilityScan = true;
  }

  // used by dlgTarget
  static bool TargetDragged(double *longitude, double *latitude);
  static bool SetTargetPan(bool dopan, int task_index);

  // used only by XCSoar.cpp on instantiation
  static LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg,
				      WPARAM wParam,LPARAM lParam);

  // use at startup
  static void SetMapRect(RECT rc) {
    MapRect = rc;
  }

  static bool isMapFullScreen(); // gui feedback

  static void RequestToggleFullScreen();
  static void RequestFullScreen(bool full);

  // used by topology store
  static void ScanVisibility(rectObj *bounds_active);
  static bool RenderTimeAvailable(); // used only by TopologyStore.cpp

  // input events or reused code
  static void Event_SetZoom(double value);
  static void Event_ScaleZoom(int vswitch);
  static void Event_Pan(int vswitch);
  static void Event_TerrainTopology(int vswitch);
  static void Event_AutoZoom(int vswitch);
  static void Event_PanCursor(int dx, int dy);

  // Drawing primitives (should go outside this class if reusable)
  // (only used by Topology)
  static void DrawBitmapIn(Canvas &canvas, const POINT &sc, const Bitmap &h);

  // used by MapWindowBase
  static DWORD DrawThread (LPVOID);

  ////////////////////////////////////////////////////////////////////
 private:

  static void DrawThreadLoop (const bool first);
  static void DrawThreadInitialise (void);

  // state
  static BOOL     Initialised;
  static bool     user_asked_redraw;

  static void     UpdateInfo(NMEA_INFO *nmea_info,
			     DERIVED_INFO *derived_info);

  // display management
  static void          RefreshMap();
  static void          SwitchZoomClimb(void);

  // state/localcopy/local data
  static double        TargetDrag_Latitude;
  static double        TargetDrag_Longitude;
  static int           TargetDrag_State;
  static POINT         Groundline[NUMTERRAINSWEEPS+1];
  static bool          LandableReachable;

  // projection
  static bool      BigZoom;
  static bool      askFullScreen;
  static bool      MapFullScreen;
  static bool      askVisibilityScan; // called only by XCSoar.cpp on
			              // settings reload
  static void      StoreRestoreFullscreen(bool);
  static void      ToggleFullScreenStart();

  static double    findMapScaleBarSize(const RECT rc);

  // other
  static DWORD     fpsTime0;
  static DWORD     timestamp_newdata;
  static void      UpdateTimeStats(bool start);

  // interface handlers
  static int ProcessVirtualKey(int X, int Y, long keytime, short vkmode);

  // display element functions

  static void ScanVisibilityWaypoints(rectObj *bounds_active);
  static void ScanVisibilityAirspace(rectObj *bounds_active);

  static void CalculateScreenPositions(POINT Orig, RECT rc,
                                       POINT *Orig_Aircraft);
  static void CalculateScreenPositionsTask();
  static void CalculateScreenPositionsWaypoints();
  static void CalculateScreenPositionsGroundline();
  static void CalculateScreenPositionsAirspace();
  static void CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE& circ);
  static void CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA& area);
  static void CalculateScreenPositionsThermalSources();
  static void CalculateWaypointReachable(void);
  static bool WaypointInTask(int ind);
  static void MapWaypointLabelSortAndRender(Canvas &canvas);

  // display renderers
  static void DrawAircraft(Canvas &canvas, const POINT Orig);
  static void DrawCrossHairs(Canvas &canvas, const POINT Orig, const RECT rc);
  static void DrawGlideCircle(Canvas &canvas, const POINT Orig, const RECT rc); // VENTA3
  static void DrawBestCruiseTrack(Canvas &canvas, const POINT Orig);
  static void DrawCompass(Canvas &canvas, const RECT rc);
  static void DrawHorizon(Canvas &canvas, const RECT rc);
  static void DrawWindAtAircraft2(Canvas &canvas, POINT Orig, RECT rc);
  static void DrawAirSpace(Canvas &canvas, const RECT rc, Canvas &buffer);
  static void DrawWaypoints(Canvas &canvas, const RECT rc);
  static void DrawWaypointsNew(Canvas &canvas, const RECT rc); // VENTA5
  static void DrawLook8000(Canvas &canvas, const RECT rc); // VENTA5
  static void DrawFlightMode(Canvas &canvas, const RECT rc);
  static void DrawGPSStatus(Canvas &canvas, const RECT rc);
  static double DrawTrail(Canvas &canvas, const POINT Orig, const RECT rc);
  static void DrawTeammate(Canvas &canvas, const RECT rc);
  static void DrawTrailFromTask(Canvas &canvas, const RECT rc, const double);
  static void DrawOffTrackIndicator(Canvas &canvas, const RECT rc);
  static void DrawProjectedTrack(Canvas &canvas, const RECT rc, const POINT Orig);
  static void DrawStartSector(Canvas &canvas, const RECT rc, POINT &Start,
                              POINT &End, int Index);
  static void DrawTask(Canvas &canvas, RECT rc, const POINT &Orig_Aircraft);
  static void DrawThermalEstimate(Canvas &canvas, const RECT rc);
  static void DrawTaskAAT(Canvas &canvas, const RECT rc, Canvas &buffer);
  static void DrawAbortedTask(Canvas &canvas, const RECT rc, const POINT Orig);

  static void DrawBearing(Canvas &canvas, const RECT rc, int bBearingValid);
  static void DrawMapScale(Canvas &canvas, const RECT rc,
			   const bool ScaleChangeFeedback);
  static void DrawMapScale2(Canvas &canvas, const RECT rc,
			    const POINT Orig_Aircraft);
  static void DrawFinalGlide(Canvas &canvas, const RECT rc);
  static void DrawThermalBand(Canvas &canvas, const RECT rc);
  static void DrawGlideThroughTerrain(Canvas &canvas, const RECT rc);
  static void DrawTerrainAbove(Canvas &hDC, const RECT rc, Canvas &buffer);
  static void DrawCDI();
  //  static void DrawSpeedToFly(HDC hDC, RECT rc);
  static void DrawFLARMTraffic(Canvas &canvas, RECT rc, POINT Orig_Aircraft);

  static void ClearAirSpace(Canvas &dc, bool fill);

  // thread, main functions
  static void RenderMapWindow(Canvas &canvas, const RECT rc);
  static void RenderMapWindowBg(Canvas &canvas, const RECT rc,
				const POINT &Orig,
				const POINT &Orig_Aircraft);
  static void UpdateCaches(const bool force=false);

  // graphics vars

  static BufferCanvas hdcDrawWindow;
  static BitmapCanvas hDCTemp;
  static BufferCanvas buffer_canvas;
  static BufferCanvas hDCMask;
};

#endif
