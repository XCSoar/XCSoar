#if !defined(AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "Sizes.h"
#include "Airspace.h"
#include "Parser.h"
#include "Calculations.h"

#define TRACKCIRCLE 3
#define NORTHCIRCLE 2
#define NORTHUP 1
#define TRACKUP 0

#define DISPLAYNAME 0
#define DISPLAYNUMBER 1
#define DISPLAYFIRSTFIVE 2
#define DISPLAYNONE 3
#define DISPLAYFIRSTTHREE 4
#define DISPLAYNAMEIFINTASK 5

#define AIRPORT				0x01
#define TURNPOINT			0x02
#define LANDPOINT			0x04
#define HOME					0x08
#define START					0x10
#define FINISH				0x20
#define RESTRICTED		0x40
#define WAYPOINTFLAG	0x80

typedef struct _WAYPOINT_INFO
{
  int Number;
	double Latitude;
	double Longitude;
	double Altitude;
	int Flags;
	TCHAR Name[NAME_SIZE + 1];
	TCHAR Comment[COMMENT_SIZE + 1];
	POINT	Screen;
	int Zoom;
	BOOL Reachable;
	double AltArivalAGL;
	BOOL Visible;
  TCHAR *Details;
} WAYPOINT;


typedef struct _SNAIL_POINT
{
	double Latitude;
	double Longitude;
	double Vario;
	POINT Screen;
} SNAIL_POINT;


void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );

typedef union{
  unsigned int AsInt;
  struct{
    unsigned Border:1;
    unsigned FillBackground:1;
    unsigned AlligneRight:1;
    unsigned Reachable:1;
    unsigned AlligneCenter:1;
  }AsFlag;
}TextInBoxMode_t;
  // mode are flags
  // bit 0 == fill background add border / 1
  // bit 1 == fill background            / 2
  // bit 2 == right alligned             / 4
  // bit 3 == landable TP label          / 8
  // bit 4 == center alligned

class MapWindow {
 public:

  static bool IsDisplayRunning();

  // 12 is number of airspace types
  static int	iAirspaceBrush[AIRSPACECLASSCOUNT];
  static int	iAirspaceColour[AIRSPACECLASSCOUNT];
  static int    iAirspaceMode[AIRSPACECLASSCOUNT];
  static HPEN hAirspacePens[AIRSPACECLASSCOUNT];
  static HPEN hSnailPens[NUMSNAILCOLORS];
  static bool bAirspaceBlackOutline;
  static HBRUSH hAirspaceBrushes[NUMAIRSPACEBRUSHES];
  static HBITMAP hAirspaceBitmap[NUMAIRSPACEBRUSHES];
  static COLORREF Colours[NUMAIRSPACECOLORS];

  static BOOL CLOSETHREAD;

  static RECT MapRect;
  static RECT MapRectBig;
  static double MapScale;
  static double MapScaleOverDistanceModify; // speedup
  static double ResMapScaleOverDistanceModify; // speedup
  static double RequestMapScale;
  static void ModifyMapScale();

  static bool MapDirty;

  static void RequestFastRefresh();
  static bool RequestFullScreen;

  static DWORD timestamp_newdata;
  static void UpdateTimeStats(bool start);

  static bool isAutoZoom();
  static bool isPan();

  static void GetLocationFromScreen(double &X, double &Y);
  static void GetLocationFromScreen(float &X, float &Y);
  static void DrawBitmapIn(const HDC hdc, const POINT &sc, const HBITMAP h);
  static void DrawBitmapX(HDC hdc, int top, int right,
		     int sizex, int sizey,
		     HDC source,
		     int offsetx, int offsety,
		     DWORD mode);
  static void RequestToggleFullScreen();
  static void RequestOnFullScreen();
  static void RequestOffFullScreen();
  static void LatLon2Screen(const float &lon, const float &lat, int &scX, int &scY);
  static void LatLon2Screen(const double &lon, const double &lat, int &scX, int &scY);
  static void LatLon2Screen(const double &lon, const double &lat, POINT &sc);

  static void CloseDrawingThread(void);
  static void CreateDrawingThread(void);
  static void SuspendDrawingThread(void);
  static void ResumeDrawingThread(void);

  static LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam);

  static bool IsMapFullScreen();

  // input events or reused code
  static void Event_SetZoom(double value);
  static void Event_ScaleZoom(int vswitch);
  static void Event_Pan(int vswitch);
  static void Event_TerrainTopology(int vswitch);
  static void Event_AutoZoom(int vswitch);
  static void Event_PanCursor(int dx, int dy);
  static bool Event_InteriorAirspaceDetails(double lon, double lat);
  static bool Event_NearestWaypointDetails(double lon, double lat, double range, bool pan);

  static void UpdateInfo(NMEA_INFO *nmea_info,
			 DERIVED_INFO *derived_info);
 private:

  static void CalculateScreenPositions(POINT Orig, RECT rc,
                                       POINT *Orig_Aircraft);
  static void CalculateScreenPositionsAirspace();
  static void CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE& circ);
  static void CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA& area);
  static void CalculateWaypointReachable(void);

  static bool PointVisible(const POINT &P, const RECT &rc);
  static bool PointVisible(const double &lon, const double &lat);

  static void DrawAircraft(HDC hdc, POINT Orig);
  static void DrawCrossHairs(HDC hdc, POINT Orig);
  static void DrawBestCruiseTrack(HDC hdc, POINT Orig);
  static void DrawCompass(HDC hdc, RECT rc);
  static void DrawWind(HDC hdc, POINT Orig, RECT rc);
  static void DrawWindAtAircraft(HDC hdc, POINT Orig, RECT rc);
  static void DrawWindAtAircraft2(HDC hdc, POINT Orig, RECT rc);
  static void DrawAirSpace(HDC hdc, RECT rc);
  static void DrawWaypoints(HDC hdc, RECT rc);
  static void DrawFlightMode(HDC hdc, RECT rc);
  static void DrawGPSStatus(HDC hdc, RECT rc);
  static void DrawTrail(HDC hdc, POINT Orig, RECT rc);
  static void DrawTask(HDC hdc, RECT rc);
  static void DrawTaskAAT(HDC hdc, RECT rc);
  static void DrawAbortedTask(HDC hdc, RECT rc, POINT Orig);
  static void DrawBearing(HDC hdc, POINT Orig);
  // static void DrawMapScale(HDC hDC,RECT rc);
  static void DrawMapScale(HDC hDC, RECT rc /* the Map Rect*/ , bool ScaleChangeFeedback);
  static void DrawMapScale2(HDC hDC,RECT rc, POINT Orig_Aircraft);
  static void DrawFinalGlide(HDC hDC,RECT rc);
  static void DrawThermalBand(HDC hDC,RECT rc);
  static void DrawGlideThroughTerrain(HDC hDC, RECT rc);
  static void DrawCDI();
  static void DrawSpeedToFly(HDC hDC, RECT rc);
  static void DrawFLARMTraffic(HDC hDC, RECT rc);

  static void DrawSolidLine(const HDC&hdc , const POINT&start , const POINT&end );
  static void TextInBox(HDC hDC, TCHAR* Value, int x, int y, int size, TextInBoxMode_t Mode, bool noOverlap=false);
  static void ToggleFullScreenStart();
  static void RefreshMap();

  static HBITMAP hDrawBitMap;
  static HBITMAP hDrawBitMapBg;
  static HBITMAP hDrawBitMapTmp;
  static HDC hdcDrawWindow;
  static HDC hdcDrawWindowBg;
  static HDC hdcScreen;
  static HDC hDCTemp;

  static HANDLE hRenderEvent;

  static rectObj screenbounds_latlon;

  static double PanX;
  static double PanY;
  static double PanXr;
  static double PanYr;

  static bool EnablePan;

  static BOOL THREADRUNNING;
  static BOOL THREADEXIT;

  static DWORD  dwDrawThreadID;
  static HANDLE hDrawThread;

  static double DisplayAngle;
  static double DisplayAircraftAngle;
  static double DrawScale;

  static int dTDisplay;

  static HBITMAP hLandable, hReachable,
    hTurnPoint, hSmall, hCruise, hClimb,
    hFinalGlide, hAutoMacCready, hTerrainWarning, hGPSStatus1, hGPSStatus2,
    hAbort, hLogger, hLoggerOff, hFLARMTraffic;

  static HBRUSH   hBackgroundBrush;

  static COLORREF BackgroundColor;

  static      HPEN hpAircraft;
  static      HPEN hpAircraftBorder;
  static      HPEN hpWind;
  static      HPEN hpWindThick;
  static      HPEN hpBearing;
  static      HPEN hpBestCruiseTrack;
  static      HPEN hpCompass;
  static      HPEN hpThermalBand;
  static      HPEN hpThermalBandGlider;
  static      HPEN hpFinalGlideAbove;
  static      HPEN hpFinalGlideBelow;
  static      HPEN hpMapScale;
  static      HPEN hpTerrainLine;
  static      HPEN hpTerrainLineBg;
  static      HPEN hpSpeedFast;
  static      HPEN hpSpeedSlow;

  static      HBRUSH hbCompass;
  static      HBRUSH hbThermalBand;
  static      HBRUSH hbBestCruiseTrack;
  static      HBRUSH hbFinalGlideBelow;
  static      HBRUSH hbFinalGlideAbove;
  static      HBRUSH hbWind;

  static RECT MapRectSmall;
  static bool MapFullScreen;

  static DWORD fpsTime0;

  ////

  static void DisplayAirspaceWarning(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top );

  static void UpdateMapScale();
  static void CalculateOrigin(RECT rc, POINT *Orig);

  static DWORD DrawThread (LPVOID);

  static void RenderMapWindow(  RECT rc);
  static void UpdateCaches(bool force=false);
  static double findMapScaleBarSize(RECT rc);

  #define SCALELISTSIZE  20
  static int ScaleListCount;
  static int ScaleCurrent;
  static double ScaleList[SCALELISTSIZE];
  static double StepMapScale(int Step);
  static double FindMapScale(double Value);

  static HPEN    hpCompassBorder;
  static HBRUSH  hBrushFlyingModeAbort;

  static HBITMAP hBmpAirportReachable;
  static HBITMAP hBmpAirportUnReachable;
  static HBITMAP hBmpFieldReachable;
  static HBITMAP hBmpFieldUnReachable;

#define MAXLABELBLOCKS 100
  static int nLabelBlocks;
  static RECT LabelBlockCoords[MAXLABELBLOCKS];

  static void StoreRestoreFullscreen(bool);
 public:

  static double GetApproxScreenRange(void);
  static int GetMapResolutionFactor();
  static bool checkLabelBlock(RECT rc);

  static HBITMAP hBmpMapScale;
  static HBITMAP hBmpCompassBg;
  static HBITMAP hBmpClimbeAbort;
  static HBITMAP hBmpUnitKm;
  static HBITMAP hBmpUnitSm;
  static HBITMAP hBmpUnitNm;
  static HBITMAP hBmpUnitM;
  static HBITMAP hBmpUnitFt;
  static HBITMAP hBmpUnitMpS;

  static bool RenderTimeAvailable();
  static bool BigZoom;
  static bool AutoZoom;

 public:
  static pointObj GlideFootPrint[NUMTERRAINSWEEPS+1];
};




extern void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );

////////////////


///////

#endif
