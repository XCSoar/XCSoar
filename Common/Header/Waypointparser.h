#if !defined(AFX_WAYPOINTPARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_WAYPOINTPARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#include <windows.h>

#include "MapWindow.h"

#define wpTerrainBoundsYes    100
#define wpTerrainBoundsYesAll 101
#define wpTerrainBoundsNo     102
#define wpTerrainBoundsNoAll  103

void ReadWayPointFile(HANDLE hFile);
void ReadWayPoints(void);
void SetHome(bool reset);
int FindNearestWayPoint(double X, double Y, double MaxRange, bool exhaustive=false);
void CloseWayPoints(void);
int dlgWaypointOutOfTerrain(TCHAR *Message);
void WaypointWriteFiles(void);
void WaypointAltitudeFromTerrain(WAYPOINT* wpt);
WAYPOINT* GrowWaypointList();
int FindMatchingWaypoint(WAYPOINT *waypoint);
void InitWayPointCalc(void); // VENTA3

#endif

