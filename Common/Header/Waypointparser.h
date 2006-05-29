#if !defined(AFX_WAYPOINTPARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_WAYPOINTPARSER_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

#define wpTerrainBoundsYes    100
#define wpTerrainBoundsYesAll 101
#define wpTerrainBoundsNo     102
#define wpTerrainBoundsNoAll  103

void ReadWayPointFile(HANDLE hFile);
void ReadWayPoints(void);
void SetHome(void);
int FindNearestWayPoint(double X, double Y, double MaxRange);
void CloseWayPoints(void);
int dlgWaypointOutOfTerrain(TCHAR *Message);

#endif

