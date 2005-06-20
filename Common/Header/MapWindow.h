#if !defined(AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_)
#define AFX_MAPWINDOW_H__695AAC30_F401_4CFF_9BD9_FE62A2A2D0D2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>


#define NAME_SIZE 20
#define COMMENT_SIZE 20

#define NORTHCIRCLE 2
#define NORTHUP 1
#define TRACKUP 0

#define DISPLAYNAME 0
#define DISPLAYNUMBER 1
#define DISPLAYFIRSTFIVE 2
#define DISPLAYNONE 3
#define DISPLAYFIRSTTHREE 4

#define ALLON 0
#define CLIP 1
#define AUTO 2
#define ALLBELOW 3

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
	double Lattitude;
	double Longditude;
	double Altitude;
	int Flags;
	TCHAR Name[NAME_SIZE + 1];
	TCHAR Comment[COMMENT_SIZE + 1];
	POINT	Screen;
	int Zoom;
	BOOL Reachable;
	BOOL Visible;
} WAYPOINT;

#define OTHER					0
#define RESTRICT      1
#define PROHIBITED    2
#define DANGER        3
#define CLASSA				4
#define CLASSB				5
#define CLASSC				6
#define CLASSD				7
#define	NOGLIDER			8
#define CTR					9
#define WAVE				10
#define AATASK					11

typedef struct _AIRSPACE_ALT
{
	double Altitude;
	double FL;
} AIRSPACE_ALT;

typedef struct _AIRSPACE_AREA
{
	TCHAR Name[NAME_SIZE + 1];
  int Type;
	AIRSPACE_ALT Base;
	AIRSPACE_ALT Top;
	unsigned FirstPoint;
	unsigned NumPoints;
	int Visible;
	double MinLattitude;
	double MaxLattitude;
	double MinLongditude;
	double MaxLongditude;

} AIRSPACE_AREA;

typedef struct _AIRSPACE_POINT
{
	double Lattitude;
	double Longditude;
	POINT Screen;
} AIRSPACE_POINT;

typedef struct _AIRSPACE_CIRCLE
{
	TCHAR Name[NAME_SIZE + 1];
  int Type;
	AIRSPACE_ALT Base;
	AIRSPACE_ALT Top;
	double Lattitude;
	double Longditude;
	double Radius;
	int ScreenX;
	int ScreenY;
	int ScreenR;
	int Visible;
} AIRSPACE_CIRCLE;

typedef struct _SNAIL_POINT
{
	double Lattitude;
	double Longditude;
	double Vario;
	POINT Screen;
} SNAIL_POINT;

#define TRAILSIZE 1000

LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,LPARAM lParam);

void CloseDrawingThread(void);
void CreateDrawingThread(void);

void DisplayAirspaceWarning(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top );

#endif