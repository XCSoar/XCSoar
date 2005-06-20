/*
XCSoar Glide Computer
Copyright (C) 2000 - 2004  M Roberts

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
*/
#include "stdafx.h"

#include "Mapwindow.h"
#include "Utils.h"
#include "McReady.h"
#include "Airspace.h"
#include "Waypointparser.h"

#include "externs.h"

#include <windows.h>
#include <math.h>

#include <tchar.h>


static DWORD DrawThread (LPVOID);

static void CalculateScreenPositions(POINT Orig, RECT rc);
static void CalculateWaypointReachable(void);

static void GetLocationFromScreen(HWND hWnd, double *X, double *Y);

int		PointVisible(POINT *P, RECT *rc);


static void DrawAircraft(HDC hdc, POINT Orig);
static void DrawWind(HDC hdc, POINT Orig);
static void DrawAirSpace(HDC hdc, RECT rc);
static void DrawWaypoints(HDC hdc, RECT rc);
static void DrawTrail(HDC hdc, POINT Orig, RECT rc);
static void DrawTask(HDC hdc, RECT rc);
static void DrawBearing(HDC hdc, POINT Orig);
static void DrawMapScale(HDC hDC,RECT rc);
static void DrawFinalGlide(HDC hDC,RECT rc);

static void DrawSolidLine(HDC , POINT , POINT );
static void DrawDashLine(HDC , INT ,POINT , POINT , COLORREF );

int		PointVisible(POINT *P, RECT *rc);

#define NUMPOINTS 2

static HBITMAP hDrawBitMap = NULL;
static HDC hdcDrawWindow = NULL;
static HDC hdcScreen = NULL;
static HDC hDCTemp = NULL;

static BOOL CLOSETHREAD = FALSE;
static BOOL	THREADRUNNING = FALSE;
static BOOL PAUSETHREAD = TRUE;
static BOOL THREADEXIT = FALSE;

DWORD  dwDrawThreadID;
HANDLE hDrawThread;

static double RequestMapScale = 10;
static double MapScale = 10; 
static double DrawScale;

static NMEA_INFO DrawInfo;
static DERIVED_INFO DerivedDrawInfo;


static HBITMAP hLandable, hReachable, hTurnPoint, hSmall;
static HBRUSH	hAirspaceBrush[17];
static COLORREF Colours[16] = {RGB(0xFF,0x00,0x00), RGB(0x00,0xFF,0x00), RGB(0x00,0x00,0xFF), RGB(0xFF,0xFF,0x00),
															 RGB(0xFF,0x00,0xFF), RGB(0x00,0xFF,0xFF), RGB(0x7F,0x00,0x00), RGB(0x00,0x7F,0x00),
															 RGB(0x00,0x00,0x7F), RGB(0x7F,0x7F,0x00), RGB(0x7F,0x00,0x7F), RGB(0x00,0x7F,0x7F),
															 RGB(0xFF,0xFF,0xFF), RGB(0xC0,0xC0,0xC0), RGB(0x7F,0x7F,0x7F), RGB(0x00,0x00,0x00)};


LRESULT CALLBACK MapWndProc (HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam)
{
	int i,j;
	TCHAR szMessageBuffer[1024];
	double X,Y;
	static DWORD dwDownTime=0, dwUpTime=0;
	
  switch (uMsg)
  {
		case WM_SIZE:
			hDrawBitMap = CreateCompatibleBitmap (hdcScreen, (int) LOWORD (lParam), (int) HIWORD (lParam));
			SelectObject(hdcDrawWindow, (HBITMAP)hDrawBitMap);
		break;

		case WM_CREATE:
			hdcScreen = GetDC(hWnd);
			hdcDrawWindow = CreateCompatibleDC(hdcScreen);
			hDCTemp = CreateCompatibleDC(hdcDrawWindow);
			for(i=0;i<16;i++)
			{
				hAirspaceBrush[i] = CreateSolidBrush(Colours[i]);
			}

			hAirspaceBrush[i] = (HBRUSH)GetStockObject(HOLLOW_BRUSH);

			hLandable=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_LANDABLE));
			hReachable=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_REACHABLE));
			hTurnPoint=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_TURNPOINT));
			hSmall=LoadBitmap(hInst, MAKEINTRESOURCE(IDB_SMALL));
     break;	


		case WM_DESTROY:
			CloseDrawingThread();
			
      ReleaseDC(hWnd, hdcScreen);
			DeleteDC(hdcDrawWindow);
			DeleteDC(hDCTemp);
			DeleteObject(hDrawBitMap);
			
			DeleteObject(hLandable);DeleteObject(hReachable);DeleteObject(hTurnPoint);DeleteObject(hSmall);

			for(i=0;i<16;i++)
			{
					DeleteObject(hAirspaceBrush[i]);
			}
			PostQuitMessage (0);
    break;
		 
		case WM_LBUTTONDOWN:
			dwDownTime = GetTickCount();
		break;

		case WM_LBUTTONUP:
			X = LOWORD(lParam); Y = HIWORD(lParam);			
			if(InfoWindowActive)
			{
				InfoWindowActive = FALSE;
				SetFocus(hWnd);
				SetWindowLong(hWndInfoWindow[InfoFocus],GWL_STYLE,WS_VISIBLE|WS_CHILD|WS_TABSTOP|SS_CENTER|SS_NOTIFY);
				break;
			}
			dwUpTime = GetTickCount(); dwDownTime = dwUpTime - dwDownTime;

			GetLocationFromScreen(hWnd, &X, &Y);

			if(dwDownTime < 1000)
			{
				i=FindNearestWayPoint(X, Y, MapScale * 500);
				if(i != -1)
				{
					wsprintf(szMessageBuffer,TEXT("Goto WayPoint\r\n%s\r\n%s"),WayPointList[i].Name,WayPointList[i].Comment);
					if(MessageBox(hWnd,szMessageBuffer ,TEXT("Go To"),MB_YESNO|MB_ICONQUESTION) == IDYES)
					{
						ActiveWayPoint = -1; AATEnabled = FALSE;
						for(j=0;j<MAXTASKPOINTS;j++)
						{
							Task[j].Index = -1;
						}
						Task[0].Index = i;
							ActiveWayPoint = 0;
					}
					SetFocus(hWnd);
          SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
					break;
				}
			}
			else
			{
				i= FindAirspaceCircle(X,Y);
				if(i != -1)
				{
					DisplayAirspaceWarning(AirspaceCircle[i].Type , AirspaceCircle[i].Name , AirspaceCircle[i].Base, AirspaceCircle[i].Top );
					break;
				}
				i= FindAirspaceArea(X,Y);
				if(i != -1)
				{
					DisplayAirspaceWarning(AirspaceArea[i].Type , AirspaceArea[i].Name , AirspaceArea[i].Base, AirspaceArea[i].Top );
					break;
				}
			}
		break;

		case WM_KEYUP:
			switch (wParam)
			{
				case VK_UP :  // SCROLL UP
						RequestMapScale *=2;
						if(RequestMapScale>160) RequestMapScale = 160; 
				break;

				case VK_DOWN: // SCROLL DOWN
						if(RequestMapScale >= 0.01)
						{
							RequestMapScale /=2;
						}
				break;
			}
		break;
  }

	return (DefWindowProc (hWnd, uMsg, wParam, lParam));
}

extern int FrameCount;

DWORD DrawThread (LPVOID lpvoid)
{
	RECT rc;
	HGDIOBJ Temp;
	POINT Orig;
	
	MapScale = RequestMapScale;
	DrawScale = MapScale/DISTANCEMODIFY;
	DrawScale = DrawScale/111000;
	DrawScale = 30/DrawScale;

	THREADRUNNING = FALSE;
	THREADEXIT = FALSE;

	GetClientRect(hWndMapWindow, &rc);
	SetBkMode(hdcDrawWindow,TRANSPARENT);
		
	while (!CLOSETHREAD) 
	{
		while(PAUSETHREAD)
		{
			THREADRUNNING = FALSE;
      SetThreadPriority(hDrawThread,THREAD_PRIORITY_BELOW_NORMAL);      
  //    MessageBeep(0);
		}
		THREADRUNNING = TRUE;

		
		memcpy(&DrawInfo,&GPS_INFO,sizeof(NMEA_INFO));
		memcpy(&DerivedDrawInfo,&CALCULATED_INFO,sizeof(DERIVED_INFO));

		if(MapScale != RequestMapScale)
		{
			MapScale = RequestMapScale;

			DrawScale = MapScale/DISTANCEMODIFY;
			DrawScale = DrawScale/111000;
			DrawScale = 30/DrawScale;
		}
/*
		if(DrawInfo.WaypointDistance > 0)
		{
			if(
				(DisplayOrientation == NORTHUP) 
				|| 
				((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == TRUE) )
			)
			{
				AutoZoomFactor = 2.5;
			}
			else
			{
				AutoZoomFactor = 4;
			}
			
			if(DrawInfo.WaypointDistance < ( AutoZoomFactor * RequestMapScale / DISTANCEMODIFY))
			{
				MapScale = DrawInfo.WaypointDistance * DISTANCEMODIFY / AutoZoomFactor;

				DrawScale = DrawInfo.WaypointDistance / AutoZoomFactor;
				DrawScale = DrawScale/111000;
				DrawScale = 30/DrawScale;
			}	
		}
*/
		Temp = SelectObject(hdcDrawWindow, GetStockObject(WHITE_BRUSH));
			

		if(InfoWindowActive)
			SelectObject(hdcDrawWindow, GetStockObject(WHITE_PEN));
		else
			SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));

		Rectangle(hdcDrawWindow,rc.left,rc.top,rc.right,rc.bottom);
			
  	Temp = SelectObject(hdcDrawWindow, GetStockObject(BLACK_BRUSH));
		Temp = SelectObject(hdcDrawWindow, GetStockObject(BLACK_PEN));


		if(
				(DisplayOrientation == TRACKUP)
				||
				((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
			)
		{
			Orig.x = (rc.left + rc.right ) /2;
			Orig.y = (rc.bottom - rc.top) - ((rc.bottom - rc.top )/6);
		}
		else
		{
			Orig.x = (rc.left + rc.right ) /2;
			Orig.y = (rc.bottom - rc.top) /2;
		}
		

		CalculateScreenPositions(Orig, rc);

		CalculateWaypointReachable();
		  
		DrawAirSpace(hdcDrawWindow, rc);

		DrawWaypoints(hdcDrawWindow,rc);

		DrawWind(hdcDrawWindow, Orig);

		DrawTask(hdcDrawWindow, rc);

		DrawBearing(hdcDrawWindow, Orig);

		DrawMapScale(hdcDrawWindow,rc );

		if(TrailActive)
			DrawTrail(hdcDrawWindow, Orig, rc);

		DrawFinalGlide(hdcDrawWindow,rc);

		DrawAircraft(hdcDrawWindow, Orig);

		BitBlt(hdcScreen,0, 0, rc.right,rc.bottom, hdcDrawWindow, 0, 0, SRCCOPY);

		FrameCount ++;
	}
  MessageBeep(0);
	THREADEXIT = TRUE;
  return 0;
}


void DrawAircraft(HDC hdc, POINT Orig)
{
	POINT Aircraft[6] = { {-15,0}, {15,0}, {0,-7}, {0,12}, {-5,10}, {5,10} };
	double dX,dY;
	int i;
	HPEN hpSolid, hpOld;


	hpSolid = (HPEN)CreatePen(PS_SOLID, 3, RGB(0,0,0));
	hpOld = (HPEN)SelectObject(hdc, hpSolid);

	for(i=0;i<6;i++)
	{
		dX = (double)Aircraft[i].x ;dY = (double)Aircraft[i].y;
		if(
				(DisplayOrientation == NORTHUP) 
				|| 
				((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == TRUE) )
			)
			rotate(&dX, &dY, DrawInfo.TrackBearing );

		Aircraft[i].x =(int)dX;  Aircraft[i].y = (int)dY;
	
		Aircraft[i].x = Orig.x + Aircraft[i].x;
		Aircraft[i].y = Orig.y + Aircraft[i].y;
	}
	DrawSolidLine(hdc,Aircraft[0],Aircraft[1]);
	DrawSolidLine(hdc,Aircraft[2],Aircraft[3]);
	DrawSolidLine(hdc,Aircraft[4],Aircraft[5]);
	
	SelectObject(hdc, hpOld);
	DeleteObject((HPEN)hpSolid);
}

void DrawWaypoints(HDC hdc, RECT rc)
{
	unsigned int i;
	TCHAR Buffer[10];
	
	for(i=0;i<NumberOfWayPoints;i++)
	{
		if(WayPointList[i].Visible )
		{
			if(MapScale > 10)
			{
				SelectObject(hDCTemp,hSmall);
			}
			else if( ((WayPointList[i].Flags & AIRPORT) == AIRPORT) || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
			{
				if(WayPointList[i].Reachable)
					SelectObject(hDCTemp,hReachable);
				else
					SelectObject(hDCTemp,hLandable);
			}
			else
			{
					SelectObject(hDCTemp,hTurnPoint);
			}	

			if((WayPointList[i].Zoom >= MapScale*10) || (WayPointList[i].Zoom == 0))
			{
				BitBlt(hdc,WayPointList[i].Screen.x-10 , WayPointList[i].Screen.y-10,20,20,
							 hDCTemp,20,0,SRCAND);
				BitBlt(hdc,WayPointList[i].Screen.x-10 , WayPointList[i].Screen.y-10,20,20,
							 hDCTemp,0,0,SRCPAINT);

			}
			
			if( ((WayPointList[i].Zoom >= MapScale*10) || (WayPointList[i].Zoom == 0)) && (MapScale <= 10))
			{	
				switch(DisplayTextType)
				{
					case DISPLAYNAME:
						ExtTextOut(hdc, WayPointList[i].Screen.x + 5, WayPointList[i].Screen.y, 0, NULL, WayPointList[i].Name, _tcslen(WayPointList[i].Name), NULL);
					break;
					case DISPLAYNUMBER:
						wsprintf(Buffer, TEXT("%d"),WayPointList[i].Number);
						ExtTextOut(hdc, WayPointList[i].Screen.x + 5, WayPointList[i].Screen.y, 0, NULL, Buffer, _tcslen(Buffer), NULL);
					break;
					case DISPLAYFIRSTFIVE:
						ExtTextOut(hdc, WayPointList[i].Screen.x + 5, WayPointList[i].Screen.y, 0, NULL, WayPointList[i].Name, 5, NULL);
					break;
					case DISPLAYFIRSTTHREE:
						ExtTextOut(hdc, WayPointList[i].Screen.x + 5, WayPointList[i].Screen.y, 0, NULL, WayPointList[i].Name, 3, NULL);
					break;
				}
			}
		}
	}
}

void DrawTask(HDC hdc, RECT rc)
{
	int i;
	double tmp;
	
	
	for(i=0;i<MAXTASKPOINTS-1;i++)
	{
		if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
		{
			DrawDashLine(hdc, 3, 
									 WayPointList[Task[i].Index].Screen, 
									 WayPointList[Task[i+1].Index].Screen, 
									 RGB(0,255,0));
		}
	}

	if((Task[0].Index >=0) &&  (Task[1].Index >=0))
	{
		if(StartLine)
		{
			DrawDashLine(hdc, 2, WayPointList[Task[0].Index].Screen, Task[0].End, RGB(127,127,127));
			DrawDashLine(hdc, 2, WayPointList[Task[0].Index].Screen, Task[0].Start , RGB(127,127,127));
		}
		tmp = StartRadius*DISTANCEMODIFY/MapScale; tmp = tmp * 30;
		SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));SelectObject(hdc, GetStockObject(BLACK_PEN));
		Circle(hdc,WayPointList[Task[0].Index].Screen.x,WayPointList[Task[0].Index].Screen.y,(int)tmp, rc); 
	}

	for(i=1;i<MAXTASKPOINTS-1;i++)
	{
		if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
		{
			if(AATEnabled == TRUE)
			{
				if(Task[i].AATType == CIRCLE)
				{
					tmp = Task[i].AATCircleRadius * DISTANCEMODIFY/MapScale;
					tmp = tmp * 30;
					SelectObject(hdc, hAirspaceBrush[iAirspaceBrush[AATASK]]);SelectObject(hdc, GetStockObject(BLACK_PEN));
					Circle(hdc,WayPointList[Task[i].Index].Screen.x,WayPointList[Task[i].Index].Screen.y,(int)tmp, rc); 
				}
				else
				{
					SelectObject(hdc, hAirspaceBrush[iAirspaceBrush[AATASK]]);SelectObject(hdc, GetStockObject(BLACK_PEN));
					DrawSolidLine(hdc,WayPointList[Task[i].Index].Screen, Task[i].AATStart);
					DrawSolidLine(hdc,WayPointList[Task[i].Index].Screen, Task[i].AATFinish);
				}
			}
			else
			{
				DrawDashLine(hdc, 2, WayPointList[Task[i].Index].Screen, Task[i].Start, RGB(127,127,127));
				DrawDashLine(hdc, 2, WayPointList[Task[i].Index].Screen, Task[i].End, RGB(127,127,127));

				if(FAISector != TRUE)
				{
					tmp = SectorRadius*DISTANCEMODIFY/MapScale;
					tmp = tmp * 30;
					SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));SelectObject(hdc, GetStockObject(BLACK_PEN));
					Circle(hdc,WayPointList[Task[i].Index].Screen.x,WayPointList[Task[i].Index].Screen.y,(int)tmp, rc); 
				}
			}
		}
	}
}

void DrawWind(HDC hdc, POINT Orig)
{
	POINT Arrow[4] = { {0,-15}, {0,-35}, {-5,-22}, {5,-22} };
	double dX,dY;
	int i;
	HPEN hpSolid, hpOld;

	hpSolid = (HPEN)CreatePen(PS_SOLID, 2, RGB(255,0,0));
	hpOld = (HPEN)SelectObject(hdc, hpSolid);

	Arrow[1].y =(long)( -15 - 5 * DerivedDrawInfo.WindSpeed );

	for(i=0;i<4;i++)
	{
		dX = (double)Arrow[i].x ;dY = (double)Arrow[i].y;
		if(
				(DisplayOrientation == TRACKUP)
				||
				((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
			)
			rotate(&dX, &dY, -1*(DrawInfo.TrackBearing - DerivedDrawInfo.WindBearing));
		else
			rotate(&dX, &dY, DerivedDrawInfo.WindBearing);

		Arrow[i].x =(int)dX;  Arrow[i].y = (int)dY;
	
		Arrow[i].x = Orig.x  + Arrow[i].x;
		Arrow[i].y = Orig.y + Arrow[i].y;
	}
	
	DrawSolidLine(hdc,Arrow[0],Arrow[1]);
	DrawSolidLine(hdc,Arrow[0],Arrow[2]);
	DrawSolidLine(hdc,Arrow[0],Arrow[3]);

	SelectObject(hdc, hpOld);
	DeleteObject((HPEN)hpSolid);
}

void DrawBearing(HDC hdc, POINT Orig)
{
	POINT Start, End;
	HPEN hpSolid, hpOld;

	hpSolid = (HPEN)CreatePen(PS_SOLID, 2, RGB(0,0,0));
	hpOld = (HPEN)SelectObject(hdc, hpSolid);

	if(ActiveWayPoint >= 0)
	{
		Start.x = WayPointList[Task[ActiveWayPoint].Index].Screen.x; 
		Start.y = WayPointList[Task[ActiveWayPoint].Index].Screen.y; 
		End.x = Orig.x;
		End.y = Orig.y;
		DrawSolidLine(hdc, Start, End);
	}

	SelectObject(hdc, hpOld);
	DeleteObject((HPEN)hpSolid);
}

void GetLocationFromScreen(HWND hWnd, double *X, double *Y)
{
	int OrigX, OrigY;
	RECT rc;

	GetClientRect(hWnd, &rc);
	if(
			(DisplayOrientation == TRACKUP)
			||
			((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
		)
	{
		OrigX = (rc.left + rc.right ) /2;
		OrigY = (rc.bottom - rc.top) - ((rc.bottom - rc.top )/6);
	}
	else
	{
		OrigX = (rc.left + rc.right ) /2;
		OrigY = (rc.bottom - rc.top) /2;
	}

	*X = (*X-OrigX)/DrawScale;
	*Y = (*Y-OrigY)/DrawScale;

	if(
			(DisplayOrientation == TRACKUP)
			||
			((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
		)
		rotate(X,Y,DrawInfo.TrackBearing);
	
	*Y = DrawInfo.Lattitude  - *Y;

	*X = *X / ((double)cos((*Y) * pi/180));

	*X = DrawInfo.Longditude + *X;
	
}

void DrawMapScale(HDC hDC,RECT rc)
{
	TCHAR Scale[5];
	POINT Start, End;
	HPEN hpSolid, hpOld;

	hpSolid = (HPEN)CreatePen(PS_SOLID, 1, RGB(0,0,0));
	hpOld = (HPEN)SelectObject(hDC, hpSolid);

	Start.x = 5; End.x = 5;
	Start.y = (19* (rc.bottom - rc.top ) )/20; End.y = Start.y - 30;
	DrawSolidLine(hDC,Start,End);

	Start.x = 10; End.x = 5;
	End.y = Start.y;
	DrawSolidLine(hDC,Start,End);

	Start.y = Start.y - 30; End.y = Start.y;
	DrawSolidLine(hDC,Start,End);

	if(MapScale <0.1)
	{
		wsprintf(Scale,TEXT("%1.2f"),MapScale);
	}
	else if(MapScale <1)
	{
		wsprintf(Scale,TEXT("%1.1f"),MapScale);
	}
	else
	{
		wsprintf(Scale,TEXT("%1.0f"),MapScale);
	}
	SetBkMode(hDC,TRANSPARENT);
	ExtTextOut(hDC, 10, End.y+7, 0, NULL, Scale, _tcslen(Scale), NULL);

	SelectObject(hDC, hpOld);
	DeleteObject((HPEN)hpSolid);
}


void DrawAirSpace(HDC hdc, RECT rc)
{
	unsigned i,j;
	POINT pt[501];

	for(i=0;i<NumberOfAirspaceCircles;i++)
	{
		if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Top.Altitude))
		{
			SelectObject(hdc, hAirspaceBrush[iAirspaceBrush[AirspaceCircle[i].Type]]);
			
			AirspaceCircle[i].Visible = Circle(hdc,AirspaceCircle[i].ScreenX ,AirspaceCircle[i].ScreenY ,AirspaceCircle[i].ScreenR ,rc);
		}
	}

	for(i=0;i<NumberOfAirspaceAreas;i++)
	{
		if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
		{
			for(j= AirspaceArea[i].FirstPoint; j < (AirspaceArea[i].NumPoints + AirspaceArea[i].FirstPoint); j++)
			{
				pt[j-AirspaceArea[i].FirstPoint].x = AirspacePoint[j].Screen.x ;
				pt[j-AirspaceArea[i].FirstPoint].y = AirspacePoint[j].Screen.y ;
			}
			
			SelectObject(hdc, hAirspaceBrush[iAirspaceBrush[AirspaceArea[i].Type]]);
			
			AirspaceArea[i].Visible= PolygonVisible(pt,AirspaceArea[i].NumPoints, rc);
			
			if(AirspaceArea[i].Visible)
				Polygon(hdc,pt,AirspaceArea[i].NumPoints);
		}
	}

	SelectObject(hdc,GetStockObject(HOLLOW_BRUSH));

	for(i=0;i<NumberOfAirspaceCircles;i++)
	{
		if(AirspaceCircle[i].Visible)
		{
			if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Top.Altitude))
			{
				Circle(hdc,AirspaceCircle[i].ScreenX ,AirspaceCircle[i].ScreenY ,AirspaceCircle[i].ScreenR, rc );
			}
		}
	}
	
	for(i=0;i<NumberOfAirspaceAreas;i++)
	{
		if(AirspaceArea[i].Visible)
		{
			if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
			{
				for(j= (int)AirspaceArea[i].FirstPoint; j < (int)(AirspaceArea[i].NumPoints+AirspaceArea[i].FirstPoint) ;j++)
				{
					pt[j-AirspaceArea[i].FirstPoint].x = AirspacePoint[j].Screen.x ;
					pt[j-AirspaceArea[i].FirstPoint].y = AirspacePoint[j].Screen.y ;
				}
				Polygon(hdc,pt,AirspaceArea[i].NumPoints);
			}
		}
	}
}


void CreateDrawingThread(void)
{
  PAUSETHREAD = FALSE; CLOSETHREAD = FALSE; THREADEXIT = FALSE;
  hDrawThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )DrawThread, 0, 0, &dwDrawThreadID);
	SetThreadPriority(hDrawThread,THREAD_PRIORITY_BELOW_NORMAL);
}


void CloseDrawingThread(void)
{
  CLOSETHREAD = TRUE;
  SetThreadPriority(hDrawThread,THREAD_PRIORITY_ABOVE_NORMAL);
	TerminateThread(hDrawThread,0);
}

void DrawFinalGlide(HDC hDC,RECT rc)
{

	POINT Scale[18] = { {5,-50 }, {14,-60 }, {23, -50}, 
											{5,-40 }, {14,-50 }, {23, -40}, 
											{5,-30 }, {14,-40 }, {23, -30}, 
											{5,-20 }, {14,-30 }, {23, -20}, 
											{5,-10 }, {14,-20 }, {23, -10}, 
											{5, 0  }, {14,-10 }, {23,   0}, 
									};

	POINT	GlideBar[5] = { {5,0},{14,-10},{23,0},{23,0},{5,0} };

	HPEN hpSolid, hpOld;
	HBRUSH hbOld, hbBrush;

	TCHAR Value[10];

	double Offset;
	int i;
	

	hpSolid = (HPEN)CreatePen(PS_SOLID, 2, RGB(0xFF,0xFF,0xFF));
	hpOld = (HPEN)SelectObject(hDC, hpSolid);
	hbOld = (HBRUSH)SelectObject(hDC, GetStockObject(WHITE_BRUSH));

	Offset = CALCULATED_INFO.TaskAltitudeDifference / 300;	
	Offset *= 20;

	if(Offset > 60) Offset = 60;
	if(Offset < -60) Offset = -60;

	
	if(Offset<0)
	{
		hbBrush=(HBRUSH)CreateSolidBrush(RGB(0xFF,0x00,0x00));
		GlideBar[1].y = 10;
	}
	else
	{
		hbBrush=(HBRUSH)CreateSolidBrush(RGB(0x00,0xFF,0x00));
	}

	(HBRUSH)SelectObject(hDC, hbBrush);

	for(i=0;i<5;i++)
	{
			GlideBar[i].y += ( (rc.bottom - rc.top )/2)-20;
	}
	GlideBar[0].y -= (int)Offset;
	GlideBar[1].y -= (int)Offset;
	GlideBar[2].y -= (int)Offset;
	
	Polygon(hDC,GlideBar,5);

	wsprintf(Value,TEXT("%1.0f"),ALTITUDEMODIFY*CALCULATED_INFO.TaskAltitudeDifference);
	SetBkMode(hDC,TRANSPARENT);
	ExtTextOut(hDC, (int)GlideBar[2].x , (int)GlideBar[2].y, 0, NULL, Value, _tcslen(Value), NULL);

	SelectObject(hDC, hbOld);
	SelectObject(hDC, hpOld);
	DeleteObject((HPEN)hpSolid);
	DeleteObject((HBRUSH)hbBrush);

}

void DrawTrail( HDC hdc, POINT Orig, RECT rc)
{
	int i;
	int P1,P2;
	HPEN	hpNew, hpOld, hpDelete;
	BYTE Red,Green,Blue;

	if(!TrailActive)
		return;

	TrailLock = TRUE;

	
	hpDelete = (HPEN)CreatePen(PS_SOLID, 2, RGB(0xFF,0xFF,0xFF));
	hpOld = (HPEN)SelectObject(hdc, hpDelete);
	
	for(i=0;i<TRAILSIZE;i++)
	{
		if( i < TRAILSIZE-1)
		{
			P1 = i; P2 = i+1;
		}
		else
		{
			P1 = i; P2 = 0;
		}

		if(SnailTrail[P1].Vario ==0)
		{
			Red = 0x7f; Green = 0x7f; Blue = 0x7f;
		}
		else if(SnailTrail[P1].Vario <0)
		{
			Red = 0xff; Green = 0x00; Blue = 0x00;
		}
		else
		{
			Red = 0x00; Green = 0xff; Blue = 0x00;
		}
		hpNew = (HPEN)CreatePen(PS_SOLID, 2, RGB((BYTE)Red,(BYTE)Green,(BYTE)Blue));
		SelectObject(hdc,hpNew);
		DeleteObject((HPEN)hpDelete);
		hpDelete = hpNew;


		if( (P2) == SnailNext) // Last Point of Trail List
		{
			if((SnailTrail[P1].Lattitude != 0) || (SnailTrail[P2].Lattitude != 0))
				DrawSolidLine(hdc,SnailTrail[P1].Screen,Orig);
		}
		else
		{
			if(PointVisible(&SnailTrail[P1].Screen ,&rc) || PointVisible(&SnailTrail[P2].Screen ,&rc) )
			{
				if(( SnailTrail[P1].Lattitude != 0) && (SnailTrail[P2].Lattitude != 0))
					DrawSolidLine(hdc,SnailTrail[P1].Screen,SnailTrail[P2].Screen);
			}
		}
	}
	TrailLock = FALSE;
	SelectObject(hdc, hpOld);
	DeleteObject((HPEN)hpDelete);
}

int PointVisible(POINT *P, RECT *rc)
{
	if(	( P->x > rc->left ) 
			&&
			( P->x < rc->right ) 
			&&
			( P->y > rc->top  ) 
			&&
			( P->y < rc->bottom  ) 
		)
		return TRUE;
	else
		return FALSE;
}


void DisplayAirspaceWarning(int Type, TCHAR *Name , AIRSPACE_ALT Base, AIRSPACE_ALT Top )
{
	TCHAR szMessageBuffer[1024];
	TCHAR szTitleBuffer[1024];

	FormatWarningString(Type, Name , Base, Top, szMessageBuffer, szTitleBuffer );
																					
	MessageBox(hWndMapWindow,szMessageBuffer ,szTitleBuffer,MB_OK|MB_ICONWARNING);
  SetWindowPos(hWndMainWindow,HWND_TOP,0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),SWP_SHOWWINDOW);
}

void CalculateScreenPositions(POINT Orig, RECT rc)
{
	unsigned int i,j;
	double X,Y;
	double tmp;

	for(i=0;i<NumberOfWayPoints;i++)
	{
		X = (double)DrawScale*(DrawInfo.Longditude - WayPointList[i].Longditude );
		X = X * (double)fastcosine(WayPointList[i].Lattitude);
		Y = (double)DrawScale*(DrawInfo.Lattitude  - WayPointList[i].Lattitude  );

		if(
				(DisplayOrientation == TRACKUP)
				||
				((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
			)
			rotate(&X, &Y, DrawInfo.TrackBearing );

		WayPointList[i].Screen.x = Orig.x - int(X);
		WayPointList[i].Screen.y = Orig.y + int(Y);

		if(PointVisible(&WayPointList[i].Screen, &rc) )
		{
			WayPointList[i].Visible = TRUE;
		}
		else
		{
			WayPointList[i].Visible = FALSE;
		}
	}


	for(i=0;i<NumberOfAirspaceCircles;i++)
	{
		if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Top.Altitude))
		{
			X = (double)DrawScale*(DrawInfo.Longditude - AirspaceCircle[i].Longditude );
			X = X * (double)fastcosine(AirspaceCircle[i].Lattitude);
			Y = (double)DrawScale*(DrawInfo.Lattitude  - AirspaceCircle[i].Lattitude  );
					
			if(
					(DisplayOrientation == TRACKUP)
					||
					((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
				)
					rotate(&X, &Y, DrawInfo.TrackBearing );				

			AirspaceCircle[i].ScreenX = Orig.x - int(X);
			AirspaceCircle[i].ScreenY = Orig.y + int(Y);

			tmp = AirspaceCircle[i].Radius * DISTANCEMODIFY/MapScale;
			tmp = tmp * 30;

			AirspaceCircle[i].ScreenR = (int)tmp;
		}
	}


	for(i=0;i<NumberOfAirspaceAreas;i++)
	{
		if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
		{
			for(j=AirspaceArea[i].FirstPoint ; j<(AirspaceArea[i].FirstPoint + AirspaceArea[i].NumPoints) ; j++)
			{
				X = (double)DrawScale*(DrawInfo.Longditude - AirspacePoint[j].Longditude);
				X = X * (double)fastcosine(AirspacePoint[j].Lattitude);
				Y = (double)DrawScale*(DrawInfo.Lattitude  - AirspacePoint[j].Lattitude  );
				
				if(
					(DisplayOrientation == TRACKUP)
					||
					((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
				)
					rotate(&X, &Y, DrawInfo.TrackBearing );

				AirspacePoint[j].Screen.x = Orig.x - int(X);
				AirspacePoint[j].Screen.y = Orig.y + int(Y);
			}
		}
	}


	if(TrailActive)
	{

		TrailLock = TRUE;

		for(i=0;i<TRAILSIZE;i++)
		{
			X = (double)DrawScale*(DrawInfo.Longditude - SnailTrail[i].Longditude );
			X = X * (double)fastcosine(SnailTrail[i].Lattitude);
			Y = (double)DrawScale*(DrawInfo.Lattitude  - SnailTrail[i].Lattitude  );

			if(
					(DisplayOrientation == TRACKUP)
					||
					((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
				)
				rotate(&X, &Y, DrawInfo.TrackBearing );

			SnailTrail[i].Screen.x = Orig.x - int(X);
			SnailTrail[i].Screen.y = Orig.y + int(Y);
		}
	}


	for(i=0;i<MAXTASKPOINTS-1;i++)
	{
		if((Task[i].Index >=0) &&  (Task[i+1].Index >=0))
		{
			X = (double)DrawScale*(DrawInfo.Longditude - Task[i].SectorEndLon);
			X = X * (double)fastcosine(WayPointList[i].Lattitude);
			Y = (double)DrawScale*(DrawInfo.Lattitude  - Task[i].SectorEndLat);
		
			if(
					(DisplayOrientation == TRACKUP)
					||
					((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
				)
				rotate(&X, &Y, DrawInfo.TrackBearing );

			Task[i].End.x  = Orig.x - int(X);
			Task[i].End.y = Orig.y + int(Y);
			
			X = (double)DrawScale*(DrawInfo.Longditude - Task[i].SectorStartLon);
			X = X * (double)fastcosine(WayPointList[i].Lattitude);
			Y = (double)DrawScale*(DrawInfo.Lattitude  - Task[i].SectorStartLat);
				
			if(
					(DisplayOrientation == TRACKUP)
					||
					((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
				)
				rotate(&X, &Y, DrawInfo.TrackBearing );

			Task[i].Start.x = Orig.x - int(X);
			Task[i].Start.y = Orig.y + int(Y);

			if((AATEnabled) && (Task[i].AATType == SECTOR))
			{
				X = (double)DrawScale*(DrawInfo.Longditude - Task[i].AATStartLon );
				X = X * (double)fastcosine(Task[i].AATStartLat);
				Y = (double)DrawScale*(DrawInfo.Lattitude  - Task[i].AATStartLat );
		
				if(
						(DisplayOrientation == TRACKUP)
						||
						((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
					)
					rotate(&X, &Y, DrawInfo.TrackBearing );

				Task[i].AATStart.x  = Orig.x - int(X);
				Task[i].AATStart.y  = Orig.y + int(Y);
			
				X = (double)DrawScale*(DrawInfo.Longditude - Task[i].AATFinishLon );
				X = X * (double)fastcosine(Task[i].AATFinishLat);
				Y = (double)DrawScale*(DrawInfo.Lattitude  - Task[i].AATFinishLat );
			
				if(
						(DisplayOrientation == TRACKUP)
						||
						((DisplayOrientation == NORTHCIRCLE) && (DerivedDrawInfo.Circling == FALSE) )	
					)
					rotate(&X, &Y, DrawInfo.TrackBearing );

				Task[i].AATFinish.x  = Orig.x - int(X);
				Task[i].AATFinish.y  = Orig.y + int(Y);
			}
		}
	}
}

void CalculateWaypointReachable(void)
{
	unsigned int i;
	double WaypointDistance, WaypointBearing,AltitudeRequired;

	for(i=0;i<NumberOfWayPoints;i++)
	{
		if(WayPointList[i].Visible )
		{
			if(  ((WayPointList[i].Flags & AIRPORT) == AIRPORT) || ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT) )
			{
				WaypointDistance = Distance(DrawInfo.Lattitude, DrawInfo.Longditude, WayPointList[i].Lattitude, WayPointList[i].Longditude);
				WaypointBearing =  Bearing(DrawInfo.Lattitude, DrawInfo.Longditude, WayPointList[i].Lattitude, WayPointList[i].Longditude);
				AltitudeRequired = McReadyAltitude(MACREADY/LIFTMODIFY,WaypointDistance,WaypointBearing, DerivedDrawInfo.WindSpeed, DerivedDrawInfo.WindBearing);
				AltitudeRequired = AltitudeRequired * (1/BUGS);	AltitudeRequired = AltitudeRequired + SAFTEYALTITUDE + WayPointList[i].Altitude ;
				AltitudeRequired = DrawInfo.Altitude - AltitudeRequired;				

				if(AltitudeRequired >=0)
					WayPointList[i].Reachable = TRUE;
				else
					WayPointList[i].Reachable = FALSE;
			}				
		}
	}
}

void DrawSolidLine(HDC hdc, POINT ptStart, POINT ptEnd)
{
	POINT pt[2];
	
	pt[0].x = ptStart.x;
	pt[0].y = ptStart.y;
	pt[1].x = ptEnd.x;
	pt[1].y = ptEnd.y;
	Polyline(hdc, pt, NUMPOINTS);
} 

void DrawDashLine(HDC hdc, INT width, POINT ptStart, POINT ptEnd, COLORREF cr)
{
	int i;
	HPEN hpDash,hpOld;
	POINT pt[2];
	//Create a dot pen
	hpDash = (HPEN)CreatePen(PS_DASH, 1, cr);
	hpOld = (HPEN)SelectObject(hdc, hpDash);

	pt[0].x = ptStart.x;
	pt[0].y = ptStart.y;
	pt[1].x = ptEnd.x;
	pt[1].y = ptEnd.y;
	
	//increment on smallest variance
	if(abs(ptStart.x - ptEnd.x) < abs(ptStart.y - ptEnd.y)){
		for (i = 0; i < width; i++){
			pt[0].x += 1;
			pt[1].x += 1;	
			Polyline(hdc, pt, NUMPOINTS);
		}	
	} else {
		for (i = 0; i < width; i++){
			pt[0].y += 1;
			pt[1].y += 1;	
			Polyline(hdc, pt, NUMPOINTS);
		}	
	}

	SelectObject(hdc, hpOld);
	DeleteObject((HPEN)hpDash);

} 
