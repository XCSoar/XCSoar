#if !defined(AFX_EXTERNS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_EXTERNS_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Sizes.h"
#include "XCSoar.h"
#include "Parser.h"
#include "Calculations.h"
#include "Mapwindow.h"
#include "Task.h"

extern HINSTANCE hInst;
extern HANDLE hPort1;            // Serial port handle
extern HANDLE hPort2;            // Serial port handle
extern HANDLE hReadThread;      // Handle to the read thread
extern LPTSTR lpszDevName;      // Communication port name
extern TCHAR strAssetNumber[REGKEYSIZE+1];
extern TCHAR strRegKey[REGKEYSIZE+1];

extern HWND hWndMainWindow;           // HWND Main Window
extern HWND hWndMapWindow;					// HWND MapWindow
extern HWND hWndCB;
extern int  CurrentInfoType;     // Used for Popup Menu Select
extern int	InfoType[NUMINFOWINDOWS]; //
extern HWND hWndInfoWindow[NUMINFOWINDOWS];
extern int	InfoFocus;
extern BOOL DisplayLocked;

extern SCREEN_INFO Data_Options[];
extern int NUMSELECTSTRINGS;

extern int HomeWaypoint;
extern WAYPOINT *WayPointList;
extern unsigned int NumberOfWayPoints;

extern TASK_POINT Task[];
extern int ActiveWayPoint;

extern bool TaskAborted;

extern int SelectedWaypoint;

extern int FAISector;
extern DWORD SectorRadius;
extern int StartLine;
extern DWORD StartRadius;

extern AIRSPACE_AREA *AirspaceArea;
extern AIRSPACE_POINT *AirspacePoint;
extern AIRSPACE_CIRCLE *AirspaceCircle;
extern unsigned int NumberOfAirspacePoints;
extern unsigned int NumberOfAirspaceAreas;
extern unsigned int NumberOfAirspaceCircles;

extern NMEA_INFO		GPS_INFO;
extern DERIVED_INFO	CALCULATED_INFO;

extern BOOL GPSCONNECT;
extern BOOL VARIOCONNECT;

extern double MACREADY;
extern bool   AutoMacReady;
extern double AccelerometerZero;

extern double SPEEDMODIFY;
extern double	LIFTMODIFY;
extern double	DISTANCEMODIFY;
extern double ALTITUDEMODIFY;

extern double SAFETYALTITUDEARRIVAL;
extern double SAFETYALTITUDEBREAKOFF;
extern double SAFETYALTITUDETERRAIN;

extern double SAFTEYSPEED;
extern double BUGS;
extern double BALLAST;
extern int POLARID;
extern double POLAR[POLARSIZE];
extern double WEIGHTS[POLARSIZE];

extern int DisplayOrientation;
extern int DisplayTextType;

extern BOOL	InfoWindowActive;

extern int AIRSPACEWARNINGS;
extern int WarningTime;
extern int AcknowledgementTime;


extern int iAirspaceBrush[];
extern int iAirspaceColour[];
extern BOOL bAirspaceBlackOutline;

extern int AltitudeMode;
extern int ClipAltitude;
extern int AltWarningMargin;
extern double QNH;

extern double pi;

extern BOOL LoggerActive;

extern BOOL TopWindow;

extern bool MapDirty;
extern bool RequestMapDirty;

extern SNAIL_POINT SnailTrail[TRAILSIZE];
extern int TrailActive;
extern	int SnailNext;
extern int TrailLock;
extern int WindUpdateMode;
extern int CircleZoom;
extern int EnableTopology;
extern int EnableTerrain;

extern int EnableSoundVario;
extern int EnableSoundTask;
extern int EnableSoundModes;
extern int SoundVolume;
extern int SoundDeadband;

extern int NettoSpeed;

extern int FinalGlideTerrain;

extern double AATTaskLength;
extern BOOL AATEnabled;


extern BOOL COMPORTCHANGED;
extern BOOL AIRSPACEFILECHANGED;
extern BOOL WAYPOINTFILECHANGED;
extern BOOL TERRAINFILECHANGED;
extern BOOL AIRFIELDFILECHANGED;
extern BOOL TOPOLOGYFILECHANGED;

extern BOOL InfoBoxesHidden;

#endif
