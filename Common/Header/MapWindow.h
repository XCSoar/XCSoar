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

class MapWindowBase {
 public:
  static Trigger  dirtyEvent;
  static void     CloseDrawingThread(void);
  static void     SuspendDrawingThread(void);
  static void     ResumeDrawingThread(void);
  static bool     IsDisplayRunning();
  static void     CreateDrawingThread(void);
 protected:
  static bool     THREADRUNNING;
  static bool     THREADEXIT;
  static DWORD    dwDrawThreadID;
  static HANDLE   hDrawThread;
};


class MapWindow: public MapWindowBase, public MapWindowProjection {
 public:
  HWND hWndMapWindow;

  /**
   * This cast operator allows existing code to use a MapWindow object
   * as if it were a HWND.
   */
  operator HWND() {
    return hWndMapWindow;
  }

  /**
   * This assignment operator allows existing code to initialize the
   * MapWindow variable.
   */
  HWND operator =(HWND hWnd) {
    return hWndMapWindow = hWnd;
  }

 public:
  // user settings

  // inter-process
  static bool ForceVisibilityScan; // called only by XCSoar.cpp on
				   // settings reload

  static bool RenderTimeAvailable(); // used only by TopologyStore.cpp

  // used by dlgTarget
  static bool TargetDragged(double *longitude, double *latitude);
  static bool SetTargetPan(bool dopan, int task_index);

  // used only by XCSoar.cpp on instantiation
  static LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg,
				      WPARAM wParam,LPARAM lParam);

  static void UpdateInfo(NMEA_INFO *nmea_info,
			 DERIVED_INFO *derived_info);

  // use at startup
  static void SetMapRect(RECT rc) {
    MapRect = rc;
  }

  // display management
  static void SwitchZoomClimb(void);

  static bool isMapFullScreen(); // gui feedback

  static void RequestToggleFullScreen();
  static void RequestFullScreen(bool full);

  // used by terrain renderer, topology and airspace
  static void ScanVisibility(rectObj *bounds_active);

  // input events or reused code
  static void Event_SetZoom(double value);
  static void Event_ScaleZoom(int vswitch);
  static void Event_Pan(int vswitch);
  static void Event_TerrainTopology(int vswitch);
  static void Event_AutoZoom(int vswitch);
  static void Event_PanCursor(int dx, int dy);

  // Drawing primitives (should go outside this class if reusable)
  // (only used by Topology)
  static void DrawBitmapIn(const HDC hdc, const POINT &sc,
			   const HBITMAP h);

  // used by MapWindowBase
  static DWORD DrawThread (LPVOID);

  ////////////////////////////////////////////////////////////////////
 private:
  // state
  static BOOL     Initialised;
  static DWORD    timestamp_newdata;
  static bool     user_asked_redraw;

  // display management
  static void     RefreshMap();

  // state/localcopy/local data
  static int           iSnailNext;
  static double        TargetDrag_Latitude;
  static double        TargetDrag_Longitude;
  static int           TargetDrag_State;
  static POINT         Groundline[NUMTERRAINSWEEPS+1];
  static bool          LandableReachable;
  static DWORD         fpsTime0;

  // projection
  static bool      BigZoom;
  static bool      askFullScreen;
  static bool      MapFullScreen;
  static void      StoreRestoreFullscreen(bool);
  static void      ToggleFullScreenStart();

  static double    findMapScaleBarSize(const RECT rc);

  // other
  static void      UpdateTimeStats(bool start);

  // display element functions

  static void ScanVisibilityTrail(rectObj *bounds_active);
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
  static void MapWaypointLabelSortAndRender(HDC hdc);

  // display renderers
  static void DrawAircraft(HDC hdc, const POINT Orig);
  static void DrawCrossHairs(HDC hdc, const POINT Orig, const RECT rc);
  static void DrawGlideCircle(HDC hdc, const POINT Orig, const RECT rc); // VENTA3
  static void DrawBestCruiseTrack(HDC hdc, const POINT Orig);
  static void DrawCompass(HDC hdc, const RECT rc);
  static void DrawHorizon(HDC hdc, const RECT rc);
  static void DrawWindAtAircraft2(HDC hdc, POINT Orig, RECT rc);
  static void DrawAirSpace(HDC hdc, const RECT rc, HDC buffer);
  static void DrawWaypoints(HDC hdc, const RECT rc);
  static void DrawWaypointsNew(HDC hdc, const RECT rc); // VENTA5
  static void DrawLook8000(HDC hdc, const RECT rc); // VENTA5
  static void DrawFlightMode(HDC hdc, const RECT rc);
  static void DrawGPSStatus(HDC hdc, const RECT rc);
  static double DrawTrail(HDC hdc, const POINT Orig, const RECT rc);
  static void DrawTeammate(HDC hdc, const RECT rc);
  static void DrawTrailFromTask(HDC hdc, const RECT rc, const double);
  static void DrawOffTrackIndicator(HDC hdc, const RECT rc);
  static void DrawProjectedTrack(HDC hdc, const RECT rc, const POINT Orig);
  static void DrawStartSector(HDC hdc, const RECT rc, POINT &Start,
                              POINT &End, int Index);
  static void DrawTask(HDC hdc, RECT rc, const POINT &Orig_Aircraft);
  static void DrawThermalEstimate(HDC hdc, const RECT rc);
  static void DrawTaskAAT(HDC hdc, const RECT rc, HDC buffer);
  static void DrawAbortedTask(HDC hdc, const RECT rc, const POINT Orig);
  static void DrawBearing(HDC hdc, const RECT rc, int bBearingValid);
  static void DrawMapScale(HDC hDC, const RECT rc,
			   const bool ScaleChangeFeedback);
  static void DrawMapScale2(HDC hDC, const RECT rc,
			    const POINT Orig_Aircraft);
  static void DrawFinalGlide(HDC hDC, const RECT rc);
  static void DrawThermalBand(HDC hDC, const RECT rc);
  static void DrawGlideThroughTerrain(HDC hDC, const RECT rc);
  static void DrawTerrainAbove(HDC hDC, const RECT rc, HDC buffer);
  static void DrawCDI();
  //  static void DrawSpeedToFly(HDC hDC, RECT rc);
  static void DrawFLARMTraffic(HDC hDC, RECT rc, POINT Orig_Aircraft);

  static void ClearAirSpace(HDC dc, bool fill);

  // thread, main functions
  static void RenderMapWindow(HDC hdc, const RECT rc);
  static void RenderMapWindowBg(HDC hdc, const RECT rc,
				const POINT &Orig,
				const POINT &Orig_Aircraft);
  static void UpdateCaches(const bool force=false);

  // graphics vars

  static HBITMAP hDrawBitMap;
  static HBITMAP hDrawBitMapTmp;
  static HBITMAP hMaskBitMap;
  static HDC hdcDrawWindow;
  static HDC hdcScreen;
  static HDC hDCTemp;
  static HDC hDCMask;
};

#endif
