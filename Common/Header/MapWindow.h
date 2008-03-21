#if !defined(AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>
#include "Sizes.h"
#include "Utils.h"
#include "Airspace.h"
#include "Parser.h"
#include "Calculations.h"

#define NORTHTRACK 4
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

typedef enum {dmNone, dmCircling, dmCruise, dmFinalGlide} DisplayMode_t;

extern DisplayMode_t UserForceDisplayMode;
extern DisplayMode_t DisplayMode;


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
  bool InTask;
  TCHAR *Details;
  bool FarVisible;
  int FileNum; // which file it is in, or -1 to delete
} WAYPOINT;


typedef struct _SNAIL_POINT
{
  float Latitude;
  float Longitude;
  float Vario;
  double Time;
  POINT Screen;
  short Colour;
  BOOL Circling;
  bool FarVisible;
} SNAIL_POINT;


void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );
void DrawDotLine(HDC, POINT , POINT , COLORREF );
void _DrawLine(HDC hdc, int PenStyle, int width, POINT ptStart, POINT ptEnd, COLORREF cr);

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
  static BOOL Initialised;
  static bool GliderCenter;

  static RECT MapRect;
  static RECT MapRectBig;
  static double MapScale;
  static double MapScaleOverDistanceModify; // speedup
  static double ResMapScaleOverDistanceModify; // speedup
  static double RequestMapScale;
  static void ModifyMapScale();
  static bool ForceVisibilityScan;

  static bool MapDirty;
  static bool LandableReachable;

  static bool DeclutterLabels;
  static bool EnableTrailDrift;
  static int GliderScreenPosition;

  static void RequestFastRefresh();
  static bool RequestFullScreen;

  static DWORD timestamp_newdata;
  static void UpdateTimeStats(bool start);

  static bool isAutoZoom();
  static bool isPan();

  static void DrawBitmapIn(const HDC hdc, const POINT &sc, const HBITMAP h);
  static void DrawBitmapX(HDC hdc, int top, int right,
		     int sizex, int sizey,
		     HDC source,
		     int offsetx, int offsety,
		     DWORD mode);
  static void RequestToggleFullScreen();
  static void RequestOnFullScreen();
  static void RequestOffFullScreen();

  static void OrigScreen2LatLon(const int &x, const int &y, 
                                double &X, double &Y);
  static void Screen2LatLon(const int &x, const int &y, double &X, double &Y);

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
  static rectObj CalculateScreenBounds(double scale);
  static void ScanVisibility(rectObj *bounds_active);

  static void SwitchZoomClimb(void);

 private:
  static void CalculateScreenPositions(POINT Orig, RECT rc, 
                                       POINT *Orig_Aircraft);
  static void CalculateScreenPositionsAirspace();
  static void CalculateScreenPositionsAirspaceCircle(AIRSPACE_CIRCLE& circ);
  static void CalculateScreenPositionsAirspaceArea(AIRSPACE_AREA& area);
  static void CalculateScreenPositionsThermalSources();
  static void CalculateWaypointReachable(void);
  
  static bool PointVisible(const POINT &P);
  static bool PointVisible(const double &lon, const double &lat);
  static bool PointInRect(const double &lon, const double &lat,
			  const rectObj &bounds);

  static void DrawAircraft(HDC hdc, POINT Orig);
  static void DrawCrossHairs(HDC hdc, POINT Orig);
  static void DrawTrack(HDC hdc, POINT Orig);
  static void DrawBestCruiseTrack(HDC hdc, POINT Orig);
  static void DrawCompass(HDC hdc, RECT rc);
  static void DrawHorizon(HDC hdc, RECT rc);
  //  static void DrawWind(HDC hdc, POINT Orig, RECT rc);
  //  static void DrawWindAtAircraft(HDC hdc, POINT Orig, RECT rc);
  static void DrawWindAtAircraft2(HDC hdc, POINT Orig, RECT rc);
  static void DrawAirSpace(HDC hdc, RECT rc);
  static void DrawWaypoints(HDC hdc, RECT rc);
  static void DrawFlightMode(HDC hdc, RECT rc);
  static void DrawGPSStatus(HDC hdc, RECT rc);
  static void DrawTrail(HDC hdc, POINT Orig, RECT rc);
  static void DrawTrailFromTask(HDC hdc, RECT rc);
  static void DrawStartSector(HDC hdc, RECT rc, POINT &Start,
                              POINT &End, int Index);
  static void DrawTask(HDC hdc, RECT rc);
  static void DrawThermalEstimate(HDC hdc, RECT rc);
  static void DrawTaskAAT(HDC hdc, RECT rc);
  static void DrawAbortedTask(HDC hdc, RECT rc, POINT Orig);
  static void DrawBearing(HDC hdc);
  static void DrawGreatCircle(HDC hdc,
                              double lon_start, double lat_start,
                              double lon_end, double lat_end);
  // static void DrawMapScale(HDC hDC,RECT rc);
  static void DrawMapScale(HDC hDC, RECT rc /* the Map Rect*/ , bool ScaleChangeFeedback);
  static void DrawMapScale2(HDC hDC,RECT rc, POINT Orig_Aircraft);
  static void DrawFinalGlide(HDC hDC,RECT rc);
  static void DrawThermalBand(HDC hDC,RECT rc);
  static void DrawGlideThroughTerrain(HDC hDC, RECT rc);
  static void DrawCDI();
  //  static void DrawSpeedToFly(HDC hDC, RECT rc);
  static void DrawFLARMTraffic(HDC hDC, RECT rc);
    
  static void DrawSolidLine(const HDC&hdc , const POINT&start , const POINT&end );
  static void TextInBox(HDC hDC, TCHAR* Value, int x, int y, int size, TextInBoxMode_t Mode, bool noOverlap=false);
  static void ToggleFullScreenStart();
  static void RefreshMap();
  static bool WaypointInTask(int ind);

 private:
  static int iSnailNext;
  static HBITMAP hDrawBitMap;
  static HBITMAP hDrawBitMapBg;
  static HBITMAP hDrawBitMapTmp;
  static HBITMAP hMaskBitMap;
  static HDC hdcDrawWindow;
  static HDC hdcDrawWindowBg;
  static HDC hdcScreen;
  static HDC hDCTemp;
  static HDC hDCMask;
  static double PanLatitude;
  static double PanLongitude;
  static bool EnablePan;
  static DWORD  dwDrawThreadID;
  static HANDLE hDrawThread;
  static double DisplayAngle;
  static double DisplayAircraftAngle;
  static double DrawScale;
  static double InvDrawScale;
  static int dTDisplay;
  static double TrailFirstTime;
  
 public:
  static HANDLE hRenderEvent;

  static rectObj screenbounds_latlon;
  
  static BOOL THREADRUNNING;
  static BOOL THREADEXIT;
  
  static double LimitMapScale(double value);

  static bool WaypointInRange(int i);

  static bool SetTargetPan(bool dopan, int task_index);

  static double GetPanLatitude() { return PanLatitude; }
  static double GetPanLongitude() { return PanLongitude; }
  static double GetInvDrawScale() { return InvDrawScale; }
  static double GetDisplayAngle() { return DisplayAngle; }

 private:
  static HBITMAP 
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
  static      HPEN hpFinalGlideBelowLandable;
  static      HPEN hpMapScale;
  static      HPEN hpTerrainLine;
  static      HPEN hpTerrainLineBg;
  static      HPEN hpSpeedFast;
  static      HPEN hpSpeedSlow;
  static      HPEN hpStartFinishThick;
  static      HPEN hpStartFinishThin;
  
  static      HBRUSH hbCompass;
  static      HBRUSH hbThermalBand;
  static      HBRUSH hbBestCruiseTrack;
  static      HBRUSH hbFinalGlideBelow;
  static      HBRUSH hbFinalGlideBelowLandable;
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

  #define SCALELISTSIZE  30
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
  static HBITMAP hBmpThermalSource;
  static HBITMAP hBmpTarget;

#define MAXLABELBLOCKS 100
  static int nLabelBlocks;
  static RECT LabelBlockCoords[MAXLABELBLOCKS];

  static void StoreRestoreFullscreen(bool);
 public:

  static double GetApproxScreenRange(void);
  static int GetMapResolutionFactor();

  static POINT GetOrigScreen(void) { return Orig_Screen; }

 private:
  static POINT Orig_Screen;
  static HBITMAP hBmpMapScale;
  static HBITMAP hBmpCompassBg;
  static HBITMAP hBmpClimbeAbort;
  static HBITMAP hBmpUnitKm;
  static HBITMAP hBmpUnitSm;
  static HBITMAP hBmpUnitNm;
  static HBITMAP hBmpUnitM;
  static HBITMAP hBmpUnitFt;
  static HBITMAP hBmpUnitMpS;

  static bool TargetPan;
  static double TargetZoomDistance;
  static int TargetPanIndex; 

 public:
  static bool isTargetPan(void);
  static bool AutoZoom;
  static bool checkLabelBlock(RECT rc);
  static bool RenderTimeAvailable();
  static bool BigZoom;
  static int SnailWidthScale; 
  static int WindArrowStyle;
 private:
  static NMEA_INFO DrawInfo;
  static DERIVED_INFO DerivedDrawInfo;

  static void CalculateOrientationTargetPan(void);
  static void CalculateOrientationNormal(void);

};

void PolygonRotateShift(POINT* poly, int n, int x, int y, 
                        double angle);

extern void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );

////////////////


///////

#endif
