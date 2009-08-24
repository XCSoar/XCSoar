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

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

#include "XCSoar.h"
#include "Airspace.h"

typedef union{
  unsigned int AsInt;
  struct{
    unsigned Border:1;
    unsigned FillBackground:1;
    unsigned AlligneRight:1;
    unsigned Reachable:1;
    unsigned AlligneCenter:1;
    unsigned WhiteBorder:1;
    unsigned WhiteBold:1;
    unsigned NoSetFont:1;  // VENTA5
    unsigned Color:3;
  }AsFlag;
} TextInBoxMode_t;
  // mode are flags
  // bit 0 == fill background add border / 1
  // bit 1 == fill background            / 2
  // bit 2 == right alligned             / 4
  // bit 3 == landable TP label          / 8
  // bit 4 == center alligned

class MapWindow {
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
  static int    iAirspaceMode[AIRSPACECLASSCOUNT];
  static int    iAirspaceBrush[AIRSPACECLASSCOUNT];
  static int    iAirspaceColour[AIRSPACECLASSCOUNT];
  static int    GliderScreenPosition;
  static bool   AutoZoom;
  static int    SnailWidthScale;
  static int    WindArrowStyle;

  // inter-process
  static bool ForceVisibilityScan;
  static bool MapDirty;
  static bool RenderTimeAvailable();
  static bool TargetDragged(double *longitude, double *latitude);

  static BOOL CLOSETHREAD;
  static BOOL THREADRUNNING;
  static BOOL THREADEXIT;
  static HANDLE hRenderEvent;
  static LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam);

  static void CloseDrawingThread(void);
  static void CreateDrawingThread(void);
  static void SuspendDrawingThread(void);
  static void ResumeDrawingThread(void);
  static bool IsDisplayRunning();

  static void UpdateInfo(NMEA_INFO *nmea_info,
			 DERIVED_INFO *derived_info);

  // display management
  static bool checkLabelBlock(RECT rc);
  static void SwitchZoomClimb(void);
  static bool isAutoZoom();
  static bool isPan();
  static bool isTargetPan(void);
  static bool IsMapFullScreen();
  static void RequestFastRefresh();
  static void RequestToggleFullScreen();
  static void RequestOnFullScreen();
  static void RequestOffFullScreen();
  static bool SetTargetPan(bool dopan, int task_index);
  static void ScanVisibility(rectObj *bounds_active);
  static bool isBigZoom(void) {
    return BigZoom;
  }
  static double GetMapScale() {
    return MapScale;
  }

  // projection
  static double GetApproxScreenRange(void);
  static int GetMapResolutionFactor();
  static POINT GetOrigScreen(void) { return Orig_Screen; }
  static rectObj screenbounds_latlon;
  static double GetPanLatitude() { return PanLatitude; }
  static double GetPanLongitude() { return PanLongitude; }
  static double GetInvDrawScale() { return InvDrawScale; }
  static double GetDisplayAngle() { return DisplayAngle; }
  static void OrigScreen2LatLon(const int &x, const int &y,
                                double &X, double &Y);
  static void Screen2LatLon(const int &x, const int &y, double &X, double &Y);

  static void LatLon2Screen(const double &lon, const double &lat, POINT &sc);
  static void LatLon2Screen(pointObj *ptin, POINT *ptout, const int n,
			    const int skip);
  static rectObj CalculateScreenBounds(double scale);
  static bool WaypointInRange(int i);

  static RECT MapRect;
  static RECT MapRectBig;

  // input events or reused code
  static void Event_SetZoom(double value);
  static void Event_ScaleZoom(int vswitch);
  static void Event_Pan(int vswitch);
  static void Event_TerrainTopology(int vswitch);
  static void Event_AutoZoom(int vswitch);
  static void Event_PanCursor(int dx, int dy);
  static bool Event_InteriorAirspaceDetails(double lon, double lat);
  static bool Event_NearestWaypointDetails(double lon, double lat,
					   double range, bool pan);

  // Drawing primitives (should go outside this class!)

  static void MapWaypointLabelSortAndRender(HDC hdc);
  static void DrawDashLine(HDC , const int , const POINT , const POINT ,
			   const COLORREF ,
			   const RECT rc);
  static void DrawBitmapIn(const HDC hdc, const POINT &sc, const HBITMAP h);

  // airspace brushes/colours
  static COLORREF GetAirspaceColour(int i);
  static HBRUSH GetAirspaceBrush(int i);
  static COLORREF GetAirspaceColourByClass(int i);
  static HBRUSH GetAirspaceBrushByClass(int i);

 private:
  // state
  static BOOL     Initialised;
  static DWORD    timestamp_newdata;

  // display management
  static void     ToggleFullScreenStart();
  static void     RefreshMap();
  static double   TargetDrag_Latitude;
  static double   TargetDrag_Longitude;

  static bool     TargetPan;
  static double   TargetZoomDistance;
  static int      TargetPanIndex;

  // state/localcopy/local data
  static int           iSnailNext;
  static NMEA_INFO     DrawInfo;
  static DERIVED_INFO  DerivedDrawInfo;
  static int           TargetDrag_State;
  static POINT         Groundline[NUMTERRAINSWEEPS+1];
  static bool          LandableReachable;
  static DWORD         fpsTime0;
  static int           nLabelBlocks;
  static RECT          LabelBlockCoords[MAXLABELBLOCKS];

  // projection
  static bool      GliderCenter;
  static double    MapScale;
  static bool      BigZoom;
  static void      ModifyMapScale();
  static double    MapScaleOverDistanceModify; // speedup
  static double    ResMapScaleOverDistanceModify; // speedup
  static double    RequestMapScale;
  static bool      RequestFullScreen;
  static void      UpdateMapScale();
  static double    findMapScaleBarSize(const RECT rc);
  static int       ScaleListCount;
  static int       ScaleCurrent;
  static double    ScaleList[SCALELISTSIZE];
  static double    StepMapScale(int Step);
  static double    FindMapScale(double Value);

  // other
  static void UpdateTimeStats(bool start);

  // helpers
  static bool PointVisible(const POINT &P);
  static bool PointVisible(const double &lon, const double &lat);
  static bool PointInRect(const double &lon, const double &lat,
			  const rectObj &bounds);

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

  // display utilities
  static void _DrawLine(HDC hdc, const int PenStyle, const int width,
	       const POINT ptStart, const POINT ptEnd,
	       const COLORREF cr, const RECT rc);
  static void DrawBitmapX(HDC hdc, int top, int right,
		     int sizex, int sizey,
		     HDC source,
		     int offsetx, int offsety,
		     DWORD mode);
  static bool TextInBox(HDC hDC, const TCHAR *Value, int x, int y, int size,
                        TextInBoxMode_t Mode, bool noOverlap=false);

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
  static void DrawGreatCircle(HDC hdc,
                              double lon_start, double lat_start,
                              double lon_end, double lat_end,
			      const RECT rc);
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
  static void DisplayAirspaceWarning(int Type, const TCHAR *Name,
                                     AIRSPACE_ALT Base, AIRSPACE_ALT Top );

  // projection
  static RECT   MapRectSmall;
  static bool   MapFullScreen;
  static void   StoreRestoreFullscreen(bool);
  static void   CalculateOrigin(const RECT rc, POINT *Orig);
  static void   CalculateOrientationTargetPan(void);
  static void   CalculateOrientationNormal(void);
  static POINT  Orig_Screen;
  static double PanLatitude;
  static double PanLongitude;
  static bool   EnablePan;
  static double DisplayAngle;
  static double DisplayAircraftAngle;
  static double DrawScale;
  static double InvDrawScale;
  static double LimitMapScale(double value);

  // thread, main functions
  static DWORD  dwDrawThreadID;
  static HANDLE hDrawThread;
  static DWORD DrawThread (LPVOID);
  static void RenderMapWindow(HDC hdc, const RECT rc);
  static void RenderMapWindowBg(HDC hdc, const RECT rc,
				const POINT &Orig,
				const POINT &Orig_Aircraft);
  static void UpdateCaches(bool force=false);

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
