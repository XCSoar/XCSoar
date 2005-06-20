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

#include "dialogs.h"
#include "resource.h"
#include "utils.h"
#include "externs.h"
#include "Port.h"

#include <commdlg.h>
#include <commctrl.h>
#include <Commdlg.h>
#include <Aygshell.h>

#include <windows.h>

#include <tchar.h>

extern TCHAR szRegistryKey[];
extern TCHAR szRegistrySpeedUnitsValue[];
extern TCHAR szRegistryDistanceUnitsValue[];
extern TCHAR szRegistryAltitudeUnitsValue[];
extern TCHAR szRegistryLiftUnitsValue[];
extern TCHAR szRegistryDisplayUpValue[];
extern TCHAR szRegistryDisplayText[];	
extern TCHAR szRegistrySafteyAltitude[];
extern TCHAR szRegistryFAISector[];
extern TCHAR szRegistrySectorRadius[];
extern TCHAR szRegistryPolarID[];
extern TCHAR szRegistryWayPointFile[];
extern TCHAR szRegistryAirspaceFile[];
extern TCHAR szRegistryTerrainFile[];
extern TCHAR szRegistryAltMode[];
extern TCHAR szRegistryClipAlt[];
extern TCHAR szRegistryAltMargin[];
extern TCHAR szRegistryRegKey[];
extern TCHAR szRegistrySnailTrail[];
extern TCHAR szRegistryStartLine[];
extern TCHAR szRegistryStartRadius[];
extern TCHAR szRegistryAirspaceWarning[];
extern TCHAR szRegistryWarningTime[];
	

void ReadWayPoints(void);
void ReadAirspace(void);
int FindIndex(HWND hWnd);
BOOL GetFromRegistry(const TCHAR *szRegValue, DWORD *pPos);
HRESULT SetToRegistry(const TCHAR *szRegValue, DWORD Pos);
void ReadNewTask(HWND hDlg);
static void LoadTask(TCHAR *FileName,HWND hDlg);
static void SaveTask(TCHAR *FileName);


LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SYSTEM_INFO si;
	OSVERSIONINFO osv;
	TCHAR Temp[1024];

	switch (message)
	{
		case WM_INITDIALOG:
			osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetSystemInfo(&si);
			GetVersionEx(&osv);
			wsprintf(Temp,TEXT("%d.%d"),osv.dwMajorVersion ,osv.dwMinorVersion );
			SetDlgItemText(hDlg,IDC_OSVERSION,Temp);
			wsprintf(Temp,TEXT("Build Date %s"),TEXT(__DATE__));
			SetDlgItemText(hDlg,IDC_EXTRA,Temp );

			switch (si.dwProcessorType )
			{
				case PROCESSOR_INTEL_386		: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Intel 386")); break;
				case PROCESSOR_INTEL_486		: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Intel 486")); break;
				case PROCESSOR_INTEL_PENTIUM: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Intel Pentium")); break;
				case PROCESSOR_MIPS_R4000		: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("MIPS R4000")); break;
				case PROCESSOR_HITACHI_SH3	: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Hitachi SH3")); break;
				case PROCESSOR_HITACHI_SH4	: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Hitachi SH4")); break;
				case PROCESSOR_STRONGARM		: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("StrongARM")); break;
				case PROCESSOR_ARM720				: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("ARM 720")); break;
				default :SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Unknown")); break;
			}
		return TRUE; 
	}
  return FALSE;
}

LRESULT CALLBACK Register(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS lpwp;

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemText(hDlg,IDC_ASSETNUMBER,strAssetNumber);	
			SetDlgItemText(hDlg,IDC_REGKEY,strRegKey);	
		return TRUE; 

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				GetDlgItemText(hDlg,IDC_REGKEY,strRegKey,sizeof(strRegKey));
				SetRegistryString(szRegistryRegKey,strRegKey);
			}
		break;
	}
  return FALSE;
}

LRESULT CALLBACK Progress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		return TRUE; 
	
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
  return FALSE;
}
LRESULT CALLBACK Menu(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			if(DisplayLocked) 
			{
				SendDlgItemMessage(hDlg,IDD_LOCK,BM_SETCHECK ,BST_CHECKED, 0);
				SetWindowText(GetDlgItem(hDlg,IDD_LOCK),TEXT("Screen Locked"));
			}
			else
			{
				SendDlgItemMessage(hDlg,IDD_LOCK,BM_SETCHECK ,BST_UNCHECKED, 0);
				SetWindowText(GetDlgItem(hDlg,IDD_LOCK),TEXT("Screen Unlocked"));
			}

		return TRUE; 
	
		case WM_COMMAND:
			EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
	}
			
  return FALSE;
}

LRESULT CALLBACK SetUnits(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD Speed = 0;
	DWORD Distance = 0;
	DWORD Lift = 0;
	DWORD Altitude = 0;

	switch (message)
	{
		case WM_INITDIALOG:
			GetFromRegistry(szRegistrySpeedUnitsValue,&Speed);
			switch(Speed)
			{
				case 0 : 
					SendDlgItemMessage(hDlg,IDC_SPEED_STATUTE,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 1 : 
					SendDlgItemMessage(hDlg,IDC_SPEED_NAUTICAL,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 2 : 
					SendDlgItemMessage(hDlg,IDC_SPEED_METRIC,BM_SETCHECK,BST_CHECKED,0);
				break;
			}
			GetFromRegistry(szRegistryDistanceUnitsValue,&Distance);
			switch(Distance)
			{
				case 0 : 
					SendDlgItemMessage(hDlg,IDC_DISTANCE_STATUTE,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 1 : 
					SendDlgItemMessage(hDlg,IDC_DISTANCE_NAUTICAL,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 2 : 
					SendDlgItemMessage(hDlg,IDC_DISTANCE_METRIC,BM_SETCHECK,BST_CHECKED,0);
				break;
			}
			GetFromRegistry(szRegistryAltitudeUnitsValue,&Altitude);
			switch(Altitude)
			{
				case 0 : 
					SendDlgItemMessage(hDlg,IDC_ALTITUDE_FEET,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 1 : 
					SendDlgItemMessage(hDlg,IDC_ALTITUDE_METRES,BM_SETCHECK,BST_CHECKED,0);
				break;
			}
			GetFromRegistry(szRegistryLiftUnitsValue,&Lift);
			switch(Lift)
			{
				case 0 : 
					SendDlgItemMessage(hDlg,IDC_LIFT_KNOTS,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 1 : 
					SendDlgItemMessage(hDlg,IDC_LIFT_METRES,BM_SETCHECK,BST_CHECKED,0);
				break;
			}
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_SPEED_STATUTE:
					SetToRegistry(szRegistrySpeedUnitsValue,0);
					SPEEDMODIFY = TOMPH;
				return TRUE;
				
				case IDC_SPEED_NAUTICAL:
					SetToRegistry(szRegistrySpeedUnitsValue,1);
					SPEEDMODIFY = TOKNOTS;
				return TRUE;
				
				case IDC_SPEED_METRIC:
					SetToRegistry(szRegistrySpeedUnitsValue,2);
					SPEEDMODIFY = TOKPH;
				return TRUE;

				case IDC_DISTANCE_STATUTE:
					SetToRegistry(szRegistryDistanceUnitsValue,0);
					DISTANCEMODIFY = TOMILES;
				return TRUE;
				
				case IDC_DISTANCE_NAUTICAL:
					SetToRegistry(szRegistryDistanceUnitsValue,1);
					DISTANCEMODIFY = TONAUTICALMILES;
				return TRUE;
				
				case IDC_DISTANCE_METRIC:
					SetToRegistry(szRegistryDistanceUnitsValue,2);
					DISTANCEMODIFY = TOKILOMETER;
				return TRUE;

				case IDC_ALTITUDE_FEET:
					SetToRegistry(szRegistryAltitudeUnitsValue,0);
					ALTITUDEMODIFY = TOFEET;
				return TRUE;
				
				case IDC_ALTITUDE_METRES:
					SetToRegistry(szRegistryAltitudeUnitsValue,1);
					ALTITUDEMODIFY = TOMETER;
				return TRUE;

				case IDC_LIFT_KNOTS:
					SetToRegistry(szRegistryLiftUnitsValue,0);
					LIFTMODIFY = TOKNOTS;
				return TRUE;
				
				case IDC_LIFT_METRES:
					SetToRegistry(szRegistryLiftUnitsValue,1);
					LIFTMODIFY = TOMETER;
				return TRUE;
			}
			break;
	}
  return FALSE;
}


LRESULT CALLBACK Select(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;
	static int ItemSelected = 0;

	switch (message)
	{
		case WM_INITDIALOG:
			// Create a Done button and size it.  
			for(i=0;i<NUMSELECTSTRINGS;i++)
			{
				SendDlgItemMessage(hDlg,IDC_CRUISE,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) Data_Options[i].Description);
				SendDlgItemMessage(hDlg,IDC_CLIMB,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) Data_Options[i].Description);
			}
			
			SendDlgItemMessage(hDlg,IDC_CLIMB,LB_SETCURSEL,(WPARAM)(CurrentInfoType&0xff),0);
			SendDlgItemMessage(hDlg,IDC_CRUISE,LB_SETCURSEL,(WPARAM)(CurrentInfoType>>8),0);

		return TRUE; 

		
		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				SHSipPreference(hDlg,SIP_FORCEDOWN);

				ItemSelected = SendDlgItemMessage(hDlg,IDC_CRUISE,LB_GETCURSEL,0,0);
				ItemSelected = ItemSelected <<8;
				ItemSelected += SendDlgItemMessage(hDlg,IDC_CLIMB,LB_GETCURSEL,0,0);
        
        EndDialog(hDlg, ItemSelected);
				return TRUE;
			}
	}
  return FALSE;
}

LRESULT CALLBACK SetPolar(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR *Polar[] = {TEXT("Vintage - Ka6"),TEXT("Club Class - ASW19"),TEXT("Standard Class - LS8"),TEXT("15M Class - ASW27"),TEXT("18M Class - LS6C"),TEXT("Open Calss ASW22")};
	int i;

	switch (message)
	{
		case WM_INITDIALOG:
			for(i=0;i<6;i++)
				SendDlgItemMessage(hDlg,IDC_POLAR,CB_ADDSTRING,0,(LPARAM)(LPCSTR)Polar[i]);

			SendDlgItemMessage(hDlg,IDC_POLAR,CB_SETCURSEL,(WPARAM) POLARID,0);
		return TRUE; 

		case WM_COMMAND:
			if (HIWORD(wParam) ==  CBN_SELCHANGE)
			{
				POLARID = SendDlgItemMessage(hDlg,IDC_POLAR,CB_GETCURSEL, 0,0);
				SetToRegistry(szRegistryPolarID,POLARID);
				CalculateNewPolarCoef();
			}
			break;
	}
  return FALSE;
}

LRESULT CALLBACK DisplayOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD Up;

	switch (message)
	{
		case WM_INITDIALOG:
			if(TrailActive == TRUE)
			{
				SendDlgItemMessage(hDlg,IDC_SNAIL,BM_SETCHECK,BST_CHECKED,0);
			}
			else
			{
				SendDlgItemMessage(hDlg,IDC_SNAIL,BM_SETCHECK,BST_UNCHECKED,0);
			}

			GetFromRegistry(szRegistryDisplayUpValue,&Up);
			switch(Up)
			{
				case TRACKUP : 
					SendDlgItemMessage(hDlg,IDC_TRACKUP,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case NORTHUP : 
					SendDlgItemMessage(hDlg,IDC_NORTHUP,BM_SETCHECK,BST_CHECKED,0);
				break;

				case NORTHCIRCLE : 
					SendDlgItemMessage(hDlg,IDC_NORTHCIRCLE,BM_SETCHECK,BST_CHECKED,0);
				break;

			}
			GetFromRegistry(szRegistryDisplayText,&Up);
			switch(Up)
			{
				case 0 : 
					SendDlgItemMessage(hDlg,IDC_WAYPOINTNAME,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case 1 : 
					SendDlgItemMessage(hDlg,IDC_WAYPOINTNUMBERS,BM_SETCHECK,BST_CHECKED,0);
				break;
				
				case 2 : 
					SendDlgItemMessage(hDlg,IDC_WAYPOINTFIRSTFIVE,BM_SETCHECK,BST_CHECKED,0);
				break;

				case 3 : 
					SendDlgItemMessage(hDlg,IDC_WAYPOINTNONE,BM_SETCHECK,BST_CHECKED,0);
				break;

				case 4 : 
					SendDlgItemMessage(hDlg,IDC_WAYPOINTFIRSTTHREE,BM_SETCHECK,BST_CHECKED,0);
				break;

			}

		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_SNAIL:
					TrailActive = !TrailActive;
					SetToRegistry(szRegistrySnailTrail,TrailActive);
				return TRUE;

				case IDC_NORTHUP:
					SetToRegistry(szRegistryDisplayUpValue,NORTHUP);
					DisplayOrientation = NORTHUP;
				return TRUE;

				case IDC_NORTHCIRCLE:
					SetToRegistry(szRegistryDisplayUpValue,NORTHCIRCLE);
					DisplayOrientation = NORTHCIRCLE;
				return TRUE;
				
				
				case IDC_TRACKUP:
					SetToRegistry(szRegistryDisplayUpValue,TRACKUP);
					DisplayOrientation = TRACKUP;
				return TRUE;

				case IDC_WAYPOINTNAME:
					SetToRegistry(szRegistryDisplayText,0);
					DisplayTextType = DISPLAYNAME;
				return TRUE;
				
				case IDC_WAYPOINTNUMBERS:
					SetToRegistry(szRegistryDisplayText,1);
					DisplayTextType = DISPLAYNUMBER;
				return TRUE;
				
				case IDC_WAYPOINTFIRSTFIVE:
					SetToRegistry(szRegistryDisplayText,2);
					DisplayTextType = DISPLAYFIRSTFIVE;
				return TRUE;
				
				case IDC_WAYPOINTNONE:
					SetToRegistry(szRegistryDisplayText,3);
					DisplayTextType = DISPLAYNONE;
				return TRUE;

				case IDC_WAYPOINTFIRSTTHREE:
					SetToRegistry(szRegistryDisplayText,4);
					DisplayTextType = DISPLAYFIRSTTHREE;
				return TRUE;
			}
			break;
	}
  return FALSE;
}


LRESULT CALLBACK SetAirspaceWarnings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS lpwp;

	switch (message)
	{
		case WM_INITDIALOG:
			if(AIRSPACEWARNINGS == TRUE)
				SendDlgItemMessage(hDlg,IDC_ENABLE,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hDlg,IDC_ENABLE,BM_SETCHECK,BST_UNCHECKED,0);

			SetDlgItemInt(hDlg,IDC_EDIT,(int)WarningTime,TRUE);

		return TRUE; 

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				WarningTime  = GetDlgItemInt(hDlg,IDC_EDIT,NULL,TRUE);
				SetToRegistry(szRegistryWarningTime,(DWORD)WarningTime);
			}
		break;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_ENABLE) 
			{
				AIRSPACEWARNINGS = !AIRSPACEWARNINGS;
				
				if(AIRSPACEWARNINGS == TRUE)
					SendDlgItemMessage(hDlg,IDC_ENABLE,BM_SETCHECK,BST_CHECKED,0);
				else
					SendDlgItemMessage(hDlg,IDC_ENABLE,BM_SETCHECK,BST_UNCHECKED,0);

				WarningTime  = GetDlgItemInt(hDlg,IDC_EDIT,NULL,TRUE);


				SetToRegistry(szRegistryAirspaceWarning,(DWORD)AIRSPACEWARNINGS);
				SetToRegistry(szRegistryWarningTime,(DWORD)WarningTime);
			}
			break;
	}
  return FALSE;
}

LRESULT CALLBACK SetTask(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned int i;
	long Selection;
	long InsertPoint;
	static int TaskSize = 0;
	static TCHAR	szFile[MAX_PATH] = TEXT("\0");
	static OPENFILENAME		ofn;
	SHINITDLGINFO shidi;

	
	switch (message)
	{
		case WM_INITDIALOG:
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_FULLSCREENNOMENUBAR;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);

			TaskSize = 0;

			// Fillout List of Waypoints
			for(i=0;i<NumberOfWayPoints;i++)
			{
				SendDlgItemMessage(hDlg,IDC_WAYPOINTS,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)WayPointList[i].Name);
			}
			// Fillout Task List
			for(i=0;i<MAXTASKPOINTS;i++)
			{
				if(Task[i].Index >=0)
				{
					SendDlgItemMessage(hDlg,IDC_TASK,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)WayPointList[Task[i].Index].Name);
					TaskSize ++;
				}
			}
			ReadNewTask(hDlg);

			if(TaskSize >=3)
				ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_SHOW);
			else
			{
				ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_HIDE);
				AATEnabled = FALSE;
			}
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case  IDOK:
					SHSipPreference(hDlg,SIP_FORCEDOWN);

					ReadNewTask(hDlg);

					EndDialog(hDlg, LOWORD(wParam));

				return TRUE;
			
				case IDC_ADD:
					
					if(TaskSize >= MAXTASKPOINTS)
						break;

					Selection = SendDlgItemMessage(hDlg,IDC_WAYPOINTS,LB_GETCURSEL,0,0L);
					if(Selection != LB_ERR)
					{
						TaskSize ++;
						InsertPoint = SendDlgItemMessage(hDlg,IDC_TASK,LB_GETCURSEL,0,0L);
						if(InsertPoint == LB_ERR)
							SendDlgItemMessage(hDlg,IDC_TASK,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)WayPointList[Selection].Name);
						else
							SendDlgItemMessage(hDlg,IDC_TASK,LB_INSERTSTRING,InsertPoint,(LPARAM)(LPCTSTR)WayPointList[Selection].Name);

						ReadNewTask(hDlg);
					}
					if(TaskSize >=3)
						ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_SHOW);
					else
					{	
						ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_HIDE);
						AATEnabled = FALSE;
					}	
				break;
			
				case IDC_REMOVE:
					Selection = SendDlgItemMessage(hDlg,IDC_TASK,LB_GETCURSEL,0,0L);
					if(Selection != LB_ERR)
					{
						SendDlgItemMessage(hDlg,IDC_TASK,LB_DELETESTRING,Selection,0L);
						TaskSize --;
						ReadNewTask(hDlg);
					}
					if(TaskSize >=3)
						ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_SHOW);
					else
					{
						ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_HIDE);
						AATEnabled = FALSE;
					}
				break;

				case IDC_LOAD:
					memset( &(ofn), 0, sizeof(ofn));
					ofn.lStructSize	= sizeof(ofn);
					ofn.hwndOwner = hDlg;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = MAX_PATH;	
					ofn.lpstrFilter = TEXT("Task Files (*.tsk)\0*.tsk\0");	
	   			ofn.lpstrTitle = TEXT("Open File");
		  		ofn.Flags = OFN_EXPLORER;

					TopWindow = FALSE;
					if(GetOpenFileName(&ofn))
					{
						LoadTask(szFile,hDlg);
						ReadNewTask(hDlg);
					}
					TopWindow = TRUE;
					
					TaskSize = 0;
					for(i=0;i<MAXTASKPOINTS;i++)
					{
						if(Task[i].Index >=0)	TaskSize ++;
					}

					if(TaskSize >=3)
						ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_SHOW);
					else
					{
						ShowWindow(GetDlgItem(hDlg,IDC_AAT),SW_HIDE);
						AATEnabled = FALSE;
					}
				break;
				
				case IDC_SAVE:
					memset( &(ofn), 0, sizeof(ofn));
					ofn.lStructSize	= sizeof(ofn);
					ofn.hwndOwner = hDlg;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = MAX_PATH;	
					ofn.lpstrFilter = TEXT("Task Files (*.tsk)\0*.tsk\0");	
	   			ofn.lpstrTitle = TEXT("Open File");
		  		ofn.Flags = OFN_EXPLORER;

					TopWindow = FALSE;
					if(GetSaveFileName( &ofn ))
						SaveTask(szFile);
					TopWindow = TRUE;
				break;

				case IDC_AAT:
					ShowWindow(hWndCB,SW_SHOW);
					SHFullScreen(hWndMainWindow,SHFS_SHOWTASKBAR);
					DialogBox(hInst, (LPCTSTR)IDD_AAT, hDlg, (DLGPROC)AAT);
					ShowWindow(hWndCB,SW_HIDE);
				break;

			}
		break;
	}
  return FALSE;
}

void ReadNewTask(HWND hDlg)
{
	int i;
	int TaskSize;
	int WayPointIndex;
	double TaskLength = 0;
	TCHAR	szTaskLength[10];
	TCHAR  WaypointID[WAY_POINT_ID_SIZE + 1];

	ActiveWayPoint = -1;

	for(i=0;i<MAXTASKPOINTS;i++)
	{
		Task[i].Index = -1;
	}
	TaskSize = SendDlgItemMessage(hDlg,IDC_TASK,LB_GETCOUNT,0,0);
	for(i=0;i<TaskSize;i++)
	{
		SendDlgItemMessage(hDlg,IDC_TASK,LB_GETTEXT,i,(LPARAM)(LPCTSTR)WaypointID);
		WayPointIndex = SendDlgItemMessage(hDlg,IDC_WAYPOINTS,LB_FINDSTRING,0,(LPARAM)(LPCTSTR)WaypointID);
		if(WayPointIndex == LB_ERR)
			break;
		else
		{
			Task[i].Index = WayPointIndex;
			if(i==0)
			{
				Task[i].Leg = 0;
				Task[i].InBound = 0;
			}
			else
			{
				Task[i].Leg = Distance(WayPointList[Task[i].Index].Lattitude,   WayPointList[Task[i].Index].Longditude,
															 WayPointList[Task[i-1].Index].Lattitude, WayPointList[Task[i-1].Index].Longditude);
				Task[i].InBound = Bearing(WayPointList[Task[i-1].Index].Lattitude,   WayPointList[Task[i-1].Index].Longditude,
															 WayPointList[Task[i].Index].Lattitude, WayPointList[Task[i].Index].Longditude);
				Task[i-1].OutBound = Task[i].InBound;
				Task[i-1].Bisector = BiSector(Task[i-1].InBound,Task[i-1].OutBound);
			}
			TaskLength += Task[i].Leg; 
		}
	}
	CalculateTaskSectors();
	CalculateAATTaskSectors();
	wsprintf(szTaskLength,TEXT("%2.1f"), DISTANCEMODIFY * TaskLength );
	SetDlgItemText(hDlg,IDC_TASKLENGTH,szTaskLength);
	if(Task[0].Index != -1)
		ActiveWayPoint = 0;
}

void LoadTask(TCHAR *szFileName, HWND hDlg)
{
	HANDLE hFile;
	TASK_POINT Temp;
	DWORD dwBytesRead;
	int i;

	TCHAR temp[] = TEXT("kjh");

	ActiveWayPoint = -1;
	for(i=0;i<MAXTASKPOINTS;i++)
	{
		Task[i].Index = -1;
	}

	SendDlgItemMessage(hDlg,IDC_TASK,LB_RESETCONTENT,0,0);

	hFile = CreateFile(szFileName,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	
	if(hFile!=INVALID_HANDLE_VALUE )
	{
		for(i=0;i<MAXTASKPOINTS;i++)
		{
			if(!ReadFile(hFile,&Temp,sizeof(TASK_POINT),&dwBytesRead,NULL))
			{
				break;
			}
			if(Temp.Index < (int)NumberOfWayPoints)
			{
				memcpy(&Task[i],&Temp, sizeof(TASK_POINT));
				/*Task[i].InBound = Temp.InBound;
				Task[i].Index = Temp.Index;
				Task[i].Leg = Temp.Leg;
				Task[i].OutBound = Temp.OutBound;
				Task[i].AATCircleRadius = Temp*/
			}
		}
		CloseHandle(hFile);
	}
	for(i=0;i<MAXTASKPOINTS;i++)
	{
		if(Task[i].Index >=0)
		{
			SendDlgItemMessage(hDlg,IDC_TASK,LB_ADDSTRING,0,(LPARAM)(LPCTSTR)WayPointList[Task[i].Index].Name);
		}
	}
	if(Task[0].Index != -1)
		ActiveWayPoint = 0;
}

void SaveTask(TCHAR *szFileName)
{
	HANDLE hFile;
	DWORD dwBytesWritten;
	
	hFile = CreateFile(szFileName,GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES)NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	
	if(hFile!=INVALID_HANDLE_VALUE )
	{
		WriteFile(hFile,&Task[0],sizeof(TASK_POINT)*MAXTASKPOINTS,&dwBytesWritten,NULL);
	}
	CloseHandle(hFile);
}

LRESULT CALLBACK FinalGlide(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Temp;
	int Temp1;
	LPWINDOWPOS lpwp;

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hDlg,IDC_SALT,(int)SAFTEYALTITUDE,FALSE);
		return TRUE; 

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				Temp1 = GetDlgItemInt(hDlg,IDC_SALT,&Temp,FALSE);
				if(Temp)
				{
					SAFTEYALTITUDE = (double)Temp1;
					SetToRegistry(szRegistrySafteyAltitude,(DWORD)SAFTEYALTITUDE);
				}
			}
			break;
	}
  return FALSE;
}

LRESULT CALLBACK SetBugsBallast(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HFONT hFont;
	int T1,T2;
	LOGFONT logfont;
	SHINITDLGINFO shidi;


	switch (message)
	{
		case WM_INITDIALOG:
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);
      
		SetDlgItemInt(hDlg,IDC_BUGS,(int)(BUGS * 100),FALSE);
			SetDlgItemInt(hDlg,IDC_BALLAST,(int)(BALLAST * 100),FALSE);
			
			memset ((char *)&logfont, 0, sizeof (logfont));
			logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
			logfont.lfHeight = 55;
			logfont.lfWidth =  15;
			logfont.lfWeight = FW_MEDIUM;

			hFont = CreateFontIndirect (&logfont);

			SendDlgItemMessage(hDlg,IDOK,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			SendDlgItemMessage(hDlg,IDC_BUGS,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
			SendDlgItemMessage(hDlg,IDC_BALLAST,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_BALLASTUP,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_BALLASTDOWN,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_BUGSUP,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_BUGSDOWN,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));

		return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				SHSipPreference(hDlg,SIP_FORCEDOWN);

				T1 = GetDlgItemInt(hDlg,IDC_BUGS,&T2,FALSE);
				if(T2)
				{
					BUGS = (double)T1;
					BUGS /=100;
					if(BUGS<0.5) BUGS = 0.5;
				}
				T1 = GetDlgItemInt(hDlg,IDC_BALLAST,&T2,FALSE);
				if(T2)
				{
					BALLAST = (double)T1;
					BALLAST /=100;
				}

				EndDialog(hDlg, LOWORD(wParam));
				DeleteObject(hFont);
				return TRUE;
			}
			if (LOWORD(wParam) == IDC_BUGSUP) 
			{
				T1 = GetDlgItemInt(hDlg,IDC_BUGS,&T2,FALSE);
				T1 += 5; if(T1>100) T1 = 50;
				SetDlgItemInt(hDlg,IDC_BUGS,(int)T1,FALSE);
			}
      if (LOWORD(wParam) == IDC_BUGSDOWN) 
			{
				T1 = GetDlgItemInt(hDlg,IDC_BUGS,&T2,FALSE);
				T1 -= 5; if(T1<50) T1 = 100;
				SetDlgItemInt(hDlg,IDC_BUGS,(int)T1,FALSE);
			}
      if (LOWORD(wParam) == IDC_BALLASTUP) 
			{
				T1 = GetDlgItemInt(hDlg,IDC_BALLAST,&T2,FALSE);
				T1 += 10; if(T1>100) T1 = 0;
				SetDlgItemInt(hDlg,IDC_BALLAST,(int)T1,FALSE);
			}
      if (LOWORD(wParam) == IDC_BALLASTDOWN) 
			{
				T1 = GetDlgItemInt(hDlg,IDC_BALLAST,&T2,FALSE);
				T1 -= 10; if(T1<0) T1 = 100;
				SetDlgItemInt(hDlg,IDC_BALLAST,(int)T1,FALSE);
			}
			
			break;
	}
  return FALSE;
}

LRESULT CALLBACK TaskSettings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Radius;
	LPWINDOWPOS lpwp;

	switch (message)
	{
		case WM_INITDIALOG:

			if(StartLine == TRUE)
				SendDlgItemMessage(hDlg,IDC_STARTLINE,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hDlg,IDC_STARTCYLINDER,BM_SETCHECK,BST_CHECKED,0);
	
			if(FAISector == TRUE)
				SendDlgItemMessage(hDlg,IDC_FAI,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hDlg,IDC_CYLINDER,BM_SETCHECK,BST_CHECKED,0);

			SetDlgItemInt(hDlg,IDC_CYLINDERRADIUS,SectorRadius,TRUE);
			SetDlgItemInt(hDlg,IDC_STARTRADIUS,StartRadius,TRUE);

		return TRUE; 

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				Radius  = GetDlgItemInt(hDlg,IDC_STARTRADIUS,0,TRUE);
				StartRadius = Radius;
				SetToRegistry(szRegistryStartRadius,Radius);
				SetToRegistry(szRegistryStartLine,StartLine);
				CalculateTaskSectors();

				Radius  = GetDlgItemInt(hDlg,IDC_CYLINDERRADIUS,0,TRUE);
				SectorRadius = Radius;
				SetToRegistry(szRegistrySectorRadius,Radius);
				SetToRegistry(szRegistryFAISector,FAISector);
			}
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_STARTLINE:
					StartLine = TRUE;
				break;
				
				case IDC_STARTCYLINDER:
					StartLine = FALSE;
				break;

				case IDC_FAI:
					FAISector = TRUE;
				break;
				
				case IDC_CYLINDER:
					FAISector = FALSE;
				break;
			}
		break;
	}
  return FALSE;
}

LRESULT CALLBACK SetFiles(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR	szAirspaceFile[MAX_PATH] = TEXT("\0");
	static OPENFILENAME		ofnAirspace;
	static TCHAR	szWaypointFile[MAX_PATH] = TEXT("\0");
	static OPENFILENAME		ofnWaypoint;
	static TCHAR	szTerrainFile[MAX_PATH] = TEXT("\0");
	static OPENFILENAME		ofnTerrain;
	static ACTIVE = FALSE;
	SHINITDLGINFO shidi;
	TCHAR szFile[MAX_PATH];

	switch (message)
	{
		case WM_INITDIALOG:
			ACTIVE = FALSE;
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_SIZEDLGFULLSCREEN;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);

			GetRegistryString(szRegistryAirspaceFile, szAirspaceFile, MAX_PATH);
			SetDlgItemText(hDlg,IDC_AIRSPACEFILE,szAirspaceFile);	
			GetRegistryString(szRegistryWayPointFile, szWaypointFile, MAX_PATH);
			SetDlgItemText(hDlg,IDC_WAYPOINTSFILE,szWaypointFile);	
			GetRegistryString(szRegistryTerrainFile, szTerrainFile, MAX_PATH);
			SetDlgItemText(hDlg,IDC_TERRAINFILE,szTerrainFile);	

			ACTIVE = TRUE;
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_AIRSPACEFILE:
					if(ACTIVE == TRUE)
					{
						if(HIWORD(wParam) == EN_UPDATE)
						{
							AIRSPACEFILECHANGED = TRUE;
							GetDlgItemText(hDlg,IDC_AIRSPACEFILE,szFile,MAX_PATH);
							SetRegistryString(szRegistryAirspaceFile,szFile);
						}
					}
				break;
				
				case IDC_WAYPOINTSFILE:
					if(ACTIVE == TRUE)
					{
						if(HIWORD(wParam) == EN_UPDATE)
						{
							WAYPOINTFILECHANGED = TRUE;
							GetDlgItemText(hDlg,IDC_WAYPOINTSFILE,szFile,MAX_PATH);
							SetRegistryString(szRegistryWayPointFile,szFile);
						}
					}
				break;

				case IDC_TERRAINFILE:
					if(ACTIVE == TRUE)
					{
						if(HIWORD(wParam) == EN_UPDATE)
						{
							TERRAINFILECHANGED = TRUE;
							GetDlgItemText(hDlg,IDC_TERRAINFILE,szFile,MAX_PATH);
							SetRegistryString(szRegistryTerrainFile,szFile);
						}
					}
				break;
			
				case IDC_BROWSEAIRSPACE:
					memset( &(ofnAirspace), 0, sizeof(ofnAirspace));
					ofnAirspace.lStructSize	= sizeof(ofnAirspace);
					ofnAirspace.hwndOwner = hDlg;
					ofnAirspace.lpstrFile = szAirspaceFile;
					ofnAirspace.nMaxFile = MAX_PATH;	
					ofnAirspace.lpstrFilter = TEXT("All Files(*.*)\0*.*\0");	
	   			ofnAirspace.lpstrTitle = TEXT("Open File");
		  		ofnAirspace.Flags = OFN_EXPLORER;

					if(GetOpenFileName(&ofnAirspace))
					{
						SetDlgItemText(hDlg,IDC_AIRSPACEFILE,szAirspaceFile);	
					}
				break;

				case IDC_BROWSETERRAIN:
					memset( &(ofnTerrain), 0, sizeof(ofnTerrain));
					ofnTerrain.lStructSize	= sizeof(ofnTerrain);
					ofnTerrain.hwndOwner = hDlg;
					ofnTerrain.lpstrFile = szTerrainFile;
					ofnTerrain.nMaxFile = MAX_PATH;	
					ofnTerrain.lpstrFilter = TEXT("All Files(*.*)\0*.*\0");	
	   			ofnTerrain.lpstrTitle = TEXT("Open File");
		  		ofnTerrain.Flags = OFN_EXPLORER;

					if(GetOpenFileName(&ofnTerrain))
					{
						SetDlgItemText(hDlg,IDC_TERRAINFILE,szTerrainFile);	
					}
				break;
				
				case IDC_BROWSEWAYPOINT:
					memset( &(ofnWaypoint), 0, sizeof(ofnWaypoint));
					ofnWaypoint.lStructSize	= sizeof(ofnWaypoint);
					ofnWaypoint.hwndOwner = hDlg;
					ofnWaypoint.lpstrFile = szWaypointFile;
					ofnWaypoint.nMaxFile = MAX_PATH;	
					ofnWaypoint.lpstrFilter = TEXT("All Files(*.*)\0*.*\0");	
	   			ofnWaypoint.lpstrTitle = TEXT("Open File");
		  		ofnWaypoint.Flags = OFN_EXPLORER;

					if(GetOpenFileName(&ofnWaypoint))
					{
						SetDlgItemText(hDlg,IDC_WAYPOINTSFILE,szWaypointFile);	
					}
				break;
			}
			break;
	}
  return FALSE;
}


LRESULT CALLBACK AirspaceAlt(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS lpwp;

	switch (message)
	{
		case WM_INITDIALOG:
			SetDlgItemInt(hDlg,IDC_CLIPALT,(int)ClipAltitude,FALSE);
	
			SetDlgItemInt(hDlg,IDC_ALTTOL,(int)AltWarningMargin,FALSE);
	
			
			switch(AltitudeMode)
			{
				case ALLON : 
					SendDlgItemMessage(hDlg,IDC_ALL,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case CLIP : 
					SendDlgItemMessage(hDlg,IDC_CLIP,BM_SETCHECK,BST_CHECKED,0);
				break;
			
				case AUTO : 
					SendDlgItemMessage(hDlg,IDC_AUTO,BM_SETCHECK,BST_CHECKED,0);
				break;

				case ALLBELOW:
					SendDlgItemMessage(hDlg,IDC_BELOW,BM_SETCHECK,BST_CHECKED,0);
				break;

			}

		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					ClipAltitude  = LOWORD(SendDlgItemMessage(hDlg,IDC_CLIPSPIN,UDM_GETPOS ,0,0L));
					SetToRegistry(szRegistryClipAlt,ClipAltitude);

					AltWarningMargin = LOWORD(SendDlgItemMessage(hDlg,IDC_MARGINSPIN,UDM_GETPOS ,0,0L));
					SetToRegistry(szRegistryAltMargin,AltWarningMargin);

					EndDialog(hDlg, TRUE);
				return TRUE;

				case IDC_ALL:
					AltitudeMode = ALLON;
					SetToRegistry(szRegistryAltMode,AltitudeMode);
				return TRUE;
				
				case IDC_CLIP:
					AltitudeMode = CLIP;
					SetToRegistry(szRegistryAltMode,AltitudeMode);
				return TRUE;

				case IDC_AUTO:
					AltitudeMode = AUTO;
					SetToRegistry(szRegistryAltMode,AltitudeMode);
				return TRUE;

				case IDC_BELOW:
					AltitudeMode = ALLBELOW;
					SetToRegistry(szRegistryAltMode,AltitudeMode);
				return TRUE;
			}
		break;

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				ClipAltitude  = GetDlgItemInt(hDlg,IDC_CLIPALT,NULL, FALSE);
				SetToRegistry(szRegistryClipAlt,ClipAltitude);

				AltWarningMargin = GetDlgItemInt(hDlg,IDC_ALTTOL,NULL, FALSE);
				SetToRegistry(szRegistryAltMargin,AltWarningMargin);
			}
		break;

	}
  return FALSE;
}

LRESULT CALLBACK AirspacePress(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	double INHg;
	TCHAR Temp[10];
	SHINITDLGINFO shidi;
 	static HFONT hFont;
	LOGFONT logfont;

	switch (message)
	{
		case WM_INITDIALOG:
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON|SHIDIF_SIZEDLG;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);

     	memset ((char *)&logfont, 0, sizeof (logfont));
			logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
			logfont.lfHeight = 25;
			logfont.lfWidth =  10;
			logfont.lfWeight = FW_MEDIUM;

			hFont = CreateFontIndirect (&logfont);

      SendDlgItemMessage(hDlg,IDOK,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_QNH,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_INHG,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_UP,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_DOWN,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC1,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC2,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC3,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));

      SetDlgItemInt(hDlg,IDC_QNH,(int)QNH,FALSE);
      INHg = (int)QNH;
			INHg = INHg /1013.2;
			INHg = INHg*29.91;
			wsprintf(Temp,TEXT("%2.2f"),INHg );
			SetDlgItemText(hDlg,IDC_INHG,Temp);

  	return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
          QNH = GetDlgItemInt(hDlg,IDC_QNH,NULL, FALSE);
					SHSipPreference(hDlg,SIP_FORCEDOWN);
					EndDialog(hDlg, TRUE);
          DeleteObject(hFont);
				return TRUE;

				case IDC_UP:
          QNH = GetDlgItemInt(hDlg,IDC_QNH,NULL, FALSE);
          QNH = QNH + 1;
					INHg = QNH;
					INHg = INHg /1013.2;
					INHg = INHg*29.91;
					wsprintf(Temp,TEXT("%2.2f"),INHg );
					SetDlgItemText(hDlg,IDC_INHG,Temp);
          SetDlgItemInt(hDlg,IDC_QNH,(int)QNH,FALSE);
				return TRUE;

 				case IDC_DOWN:
          QNH = GetDlgItemInt(hDlg,IDC_QNH,NULL, FALSE);
          QNH = QNH - 1;
					INHg = QNH;
					INHg = INHg /1013.2;
					INHg = INHg*29.91;
					wsprintf(Temp,TEXT("%2.2f"),INHg );
					SetDlgItemText(hDlg,IDC_INHG,Temp);
          SetDlgItemInt(hDlg,IDC_QNH,(int)QNH,FALSE);
				return TRUE;

				
			}
		break;
	}
  return FALSE;
}

int NewColor;

LRESULT CALLBACK MapColour(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hDlg, TRUE);
				return TRUE;

				case IDC_CLASSA:
					NewColor = iAirspaceBrush[CLASSA];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[CLASSA] = NewColor;
					SetRegistryColour(CLASSA,NewColor);
				return TRUE;
				
				case IDC_CLASSB:
					NewColor = iAirspaceBrush[CLASSB];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[CLASSB] = NewColor;
					SetRegistryColour(CLASSB,NewColor);
				return TRUE;
				
				case IDC_CLASSC:
					NewColor = iAirspaceBrush[CLASSC];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[CLASSC] = NewColor;
					SetRegistryColour(CLASSC,NewColor);
				return TRUE;

				case IDC_CLASSD:
					NewColor = iAirspaceBrush[CLASSD];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[CLASSD] = NewColor;
					SetRegistryColour(CLASSD,NewColor);
				return TRUE;

				case IDC_PROHIBITED:
					NewColor = iAirspaceBrush[PROHIBITED];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[PROHIBITED] = NewColor;
					SetRegistryColour(PROHIBITED,NewColor);
				return TRUE;

				case IDC_DANGER:
					NewColor = iAirspaceBrush[DANGER];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[DANGER] = NewColor;
					SetRegistryColour(DANGER,NewColor);
				return TRUE;

				case IDC_RESTRICTED:
					NewColor = iAirspaceBrush[RESTRICT];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[RESTRICT] = NewColor;
					SetRegistryColour(RESTRICT,NewColor);
				return TRUE;
				
				case IDC_CTR:
					NewColor = iAirspaceBrush[CTR];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[CTR] = NewColor;
					SetRegistryColour(CTR,NewColor);
				return TRUE;

				case IDC_NOGLIDER:
					NewColor = iAirspaceBrush[NOGLIDER];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[NOGLIDER] = NewColor;
					SetRegistryColour(NOGLIDER,NewColor);
				return TRUE;

				case IDC_WAVE:
					NewColor = iAirspaceBrush[WAVE];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[WAVE] = NewColor;
					SetRegistryColour(WAVE,NewColor);
				return TRUE;

				case IDC_OTHER:
					NewColor = iAirspaceBrush[OTHER];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[OTHER] = NewColor;
					SetRegistryColour(OTHER,NewColor);
				return TRUE;

				case IDC_AAT:
					NewColor = iAirspaceBrush[AATASK];
					DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);				
					iAirspaceBrush[AATASK] = NewColor;
					SetRegistryColour(AATASK,NewColor);
				return TRUE;

			}
			break;
	}
  return FALSE;
}

LRESULT CALLBACK ColourSelect(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static CHOOSECOLOR cc;
	long Temp;

	switch (message)
	{
		case WM_INITDIALOG:
			Temp = GetWindowLong(GetDlgItem(hDlg,IDC_BM1+NewColor),GWL_STYLE);
			Temp = Temp | WS_BORDER;
			SetWindowLong(GetDlgItem(hDlg,IDC_BM1+NewColor),GWL_STYLE,Temp);
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hDlg, TRUE);
				return TRUE;

				case IDC_BM1:
					NewColor = 0;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM2:
					NewColor = 1;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM3:
					NewColor = 2;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM4:
					NewColor = 3;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM5:
					NewColor = 4;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM6:
					NewColor = 5;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM7:
					NewColor = 6;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM8:
					NewColor = 7;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM9:
					NewColor = 8;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM10:
					NewColor = 9;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM11:
					NewColor = 10;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM12:
					NewColor = 11;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM13:
					NewColor = 12;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM14:
					NewColor = 13;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM15:
					NewColor = 14;
					EndDialog(hDlg, TRUE);
				return TRUE;

				case IDC_BM16:
					NewColor = 15;
					EndDialog(hDlg, TRUE);
				return TRUE;
				
				case IDC_BM17:
					NewColor = 16;
					EndDialog(hDlg, TRUE);
				return TRUE;
			}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK COMMOptions(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	TCHAR *COMMPort[] = {TEXT("COM1"),TEXT("COM2"),TEXT("COM3"),TEXT("COM4"),TEXT("COM5"),TEXT("COM6"),TEXT("COM7"),TEXT("COM8"),TEXT("COM9"),TEXT("COM10")};
	TCHAR *Speed[] = {TEXT("1200"),TEXT("2400"),TEXT("4800"),TEXT("9600"),TEXT("19200"),TEXT("38400"),TEXT("57600"),TEXT("115200")};
	DWORD	dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};
	int i;
	DWORD dwPortIndex = 0;
	DWORD dwSpeedIndex = 2;

	switch (message)
	{
		case WM_INITDIALOG:
			for(i=0;i<10;i++)
				SendDlgItemMessage(hDlg,IDC_COMMPORT,CB_ADDSTRING,0,(LPARAM)(LPCSTR)COMMPort[i]);
			
			for(i=0;i<8;i++)
				SendDlgItemMessage(hDlg,IDC_PORTSPEED,CB_ADDSTRING,0,(LPARAM)(LPCSTR)Speed[i]);

			ReadPortSettings(&dwPortIndex,&dwSpeedIndex);

			SendDlgItemMessage(hDlg,IDC_COMMPORT,CB_SETCURSEL,(WPARAM) dwPortIndex,0);
			SendDlgItemMessage(hDlg,IDC_PORTSPEED,CB_SETCURSEL,(WPARAM) dwSpeedIndex,0);

		return TRUE; 
		
		case WM_COMMAND:
			if (HIWORD(wParam) ==  CBN_SELCHANGE)
			{
				COMPORTCHANGED = TRUE;
				dwPortIndex = SendDlgItemMessage(hDlg,IDC_COMMPORT,CB_GETCURSEL, 0,0);
				dwSpeedIndex = SendDlgItemMessage(hDlg,IDC_PORTSPEED,CB_GETCURSEL, 0,0);
				WritePortSettings(dwPortIndex,dwSpeedIndex);
			}
			break;
	}
  return FALSE;
}

static OPENFILENAME		profileofn;

LRESULT CALLBACK LoadProfile(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR	szFile[MAX_PATH] = TEXT("\0");

	switch (message)
	{
		case WM_INITDIALOG:
			memset( &(profileofn), 0, sizeof(profileofn));
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case  IDC_LOAD:
					GetDlgItemText(hDlg,IDC_FILE,szFile,MAX_PATH);
					ReadProfile(hDlg,szFile);
					EndDialog(hDlg, TRUE);
				return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, FALSE);
				return TRUE;

				case IDC_BROWSE:
					memset( &(profileofn), 0, sizeof(profileofn));
					profileofn.lStructSize	= sizeof(profileofn);
					profileofn.hwndOwner = hDlg;
					profileofn.lpstrFile = szFile;
					profileofn.nMaxFile = MAX_PATH;	
					profileofn.lpstrFilter = TEXT("Profiles (*.prf)\0*.prf\0");	
	   			profileofn.lpstrTitle = TEXT("Open Profile..");
		  		profileofn.Flags = OFN_EXPLORER;

					TopWindow = FALSE;
					if(GetOpenFileName(&profileofn))
					{
						SetDlgItemText(hDlg,IDC_FILE,profileofn.lpstrFile );	
					}
					TopWindow = TRUE;
				break;

			}
			break;
	}
  return FALSE;
}


LRESULT CALLBACK SaveProfile(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR	szFile[MAX_PATH] = TEXT("\0");

	switch (message)
	{
		case WM_INITDIALOG:
			memset( &(profileofn), 0, sizeof(profileofn));
		return TRUE; 

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case  IDC_SAVE:
					GetDlgItemText(hDlg,IDC_FILE,szFile,MAX_PATH);
					WriteProfile(hDlg,szFile);


					EndDialog(hDlg, TRUE);
				return TRUE;

				case IDCANCEL:
					GetDlgItemText(hDlg,IDC_FILE,szFile,MAX_PATH);
					WriteProfile(hDlg,szFile);

					EndDialog(hDlg, FALSE);
				return TRUE;

				case IDC_BROWSE:
					memset( &(profileofn), 0, sizeof(profileofn));
					profileofn.lStructSize	= sizeof(profileofn);
					profileofn.hwndOwner = hDlg;
					profileofn.lpstrFile = szFile;
					profileofn.nMaxFile = MAX_PATH;	
					profileofn.lpstrFilter = TEXT("Profiles (*.prf)\0*.prf\0");	
	   			profileofn.lpstrTitle = TEXT("Save Profile as..");
		  		profileofn.Flags = OFN_EXPLORER;

					TopWindow = FALSE;
					if(GetSaveFileName( &profileofn ))
						SetDlgItemText(hDlg,IDC_FILE, profileofn.lpstrFile );
					TopWindow = TRUE;
				break;
			}
			break;
	}
  return FALSE;
}



#define NUMPAGES 14
static const TCHAR *szSettingsTab[] = {TEXT("About"),TEXT("Register"),TEXT("Airspace Display"),TEXT("Airspace Colours"),TEXT("Airspace Warnings"),TEXT("COMM"),TEXT("Display"),TEXT("Files"),TEXT("Final Glide"),TEXT("Polar"),TEXT("Profile Load"), TEXT("Profile Save"), TEXT("Task"),TEXT("Units")};
static HWND hTabPage[20];
int FindIndex(HWND hWnd)
{
	int i;

	for(i=0;i<20;i++)
	{
		if(hTabPage[i] == hWnd)
			return i;
	}

	return 0;
}

LRESULT CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SHINITDLGINFO shidi;
	int i;
	static int LastShow = 0, NextShow = 0;
	
	switch (message)
	{
		case WM_INITDIALOG:
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON|SHIDIF_SIZEDLG;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);
			
			for(i=0;i<NUMPAGES;i++)
			{
				SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_ADDSTRING,0,(LPARAM)(LPCSTR)szSettingsTab[i]);
			}
			SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_SETCURSEL,LastShow,0);

			hTabPage[0] = CreateDialog(hInst, (LPCTSTR)IDD_ABOUTBOX, hDlg, (DLGPROC)About);
			hTabPage[1] = CreateDialog(hInst, (LPCTSTR)IDD_REGISTER, hDlg, (DLGPROC)Register);
			hTabPage[2] = CreateDialog(hInst, (LPCTSTR)IDD_AIRSPACEALT, hDlg, (DLGPROC)AirspaceAlt);
			hTabPage[3] = CreateDialog(hInst, (LPCTSTR)IDD_COLOUR, hDlg, (DLGPROC)MapColour);
			hTabPage[4] = CreateDialog(hInst, (LPCTSTR)IDD_AIRSPACEWARN, hDlg, (DLGPROC)SetAirspaceWarnings);
			hTabPage[5] = CreateDialog(hInst, (LPCTSTR)IDD_COMM, hDlg, (DLGPROC)COMMOptions);
			hTabPage[6] = CreateDialog(hInst, (LPCTSTR)IDD_DISPLAY, hDlg, (DLGPROC)DisplayOptions);
			hTabPage[7] = CreateDialog(hInst, (LPCTSTR)IDD_FILES, hDlg, (DLGPROC)SetFiles);
			hTabPage[8] = CreateDialog(hInst, (LPCTSTR)IDD_FGLIDE, hDlg, (DLGPROC)FinalGlide);
			hTabPage[9] = CreateDialog(hInst, (LPCTSTR)IDD_POLAR, hDlg, (DLGPROC)SetPolar);
			hTabPage[10] = CreateDialog(hInst, (LPCTSTR)IDD_PROFILELOAD, hDlg, (DLGPROC)LoadProfile);
			hTabPage[11] = CreateDialog(hInst, (LPCTSTR)IDD_PROFILESAVE, hDlg, (DLGPROC)SaveProfile);
			hTabPage[12] = CreateDialog(hInst, (LPCTSTR)IDD_TASKSETTINGS, hDlg, (DLGPROC)TaskSettings);
			hTabPage[13] = CreateDialog(hInst, (LPCTSTR)IDD_UNITS, hDlg, (DLGPROC)SetUnits);

			MoveWindow(hTabPage[LastShow],0,30,240,300,TRUE);
			ShowWindow(hTabPage[LastShow],SW_SHOW);
		return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				SHSipPreference(hDlg,SIP_FORCEDOWN);

				for(i=0;i<NUMPAGES;i++)
				{
					DestroyWindow(hTabPage[i]);
				}
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}

			if(LOWORD(wParam) == IDC_SETTINGS)
			{
				if(HIWORD(wParam) == CBN_SELCHANGE)
				{
					NextShow = SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_GETCURSEL,0,0);
					ShowWindow(hTabPage[LastShow],SW_HIDE);UpdateWindow(hTabPage[LastShow]);
					MoveWindow(hTabPage[NextShow],0,30,240,300,TRUE);
					ShowWindow(hTabPage[NextShow],SW_SHOW);UpdateWindow(hTabPage[NextShow]);
					LastShow = NextShow;
					UpdateWindow(hDlg);
				}

			}
		break;
	}
  return FALSE;
}


LRESULT CALLBACK AAT(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SHINITDLGINFO shidi;
	int i;
	static int LastShow = 0, NextShow = 0;
	static int TaskSize;
	
	switch (message)
	{
		case WM_INITDIALOG:
			shidi.dwMask = SHIDIM_FLAGS;
			shidi.dwFlags = SHIDIF_DONEBUTTON|SHIDIF_SIZEDLG;
			shidi.hDlg = hDlg;
			SHInitDialog(&shidi);


			TaskSize = 0;
			for(i=0;i<MAXTASKPOINTS;i++)
			{
				if(Task[i].Index >=0)
				{
					SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_ADDSTRING,0,(LPARAM)(LPCSTR)WayPointList[Task[i].Index].Name);
					TaskSize ++;
				}
			}
			SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_SETCURSEL,LastShow,0);

			hTabPage[0] = CreateDialog(hInst, (LPCTSTR)IDD_AATSTART, hDlg, (DLGPROC)AATStart);
			for(i=1;i<TaskSize-1;i++)
			{
				hTabPage[i] = CreateDialogParam(hInst, (LPCTSTR)IDD_AATTURN, hDlg, (DLGPROC)AATTurn,i);
			}
			hTabPage[TaskSize-1] = CreateDialog(hInst, (LPCTSTR)IDD_AATFINISH, hDlg, (DLGPROC)About);
			
			
			MoveWindow(hTabPage[LastShow],0,30,240,300,TRUE);
			ShowWindow(hTabPage[LastShow],SW_SHOW);
		return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK) 
			{
				SHSipPreference(hDlg,SIP_FORCEDOWN);
				CalculateAATTaskSectors();
				for(i=0;i<TaskSize;i++)
				{
					DestroyWindow(hTabPage[i]);
				}
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}

			if(LOWORD(wParam) == IDC_SETTINGS)
			{
				if(HIWORD(wParam) == CBN_SELCHANGE)
				{
					NextShow = SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_GETCURSEL,0,0);
					ShowWindow(hTabPage[LastShow],SW_HIDE);UpdateWindow(hTabPage[LastShow]);
					MoveWindow(hTabPage[NextShow],0,30,240,300,TRUE);
					ShowWindow(hTabPage[NextShow],SW_SHOW);UpdateWindow(hTabPage[NextShow]);
					LastShow = NextShow;
					UpdateWindow(hDlg);
				}

			}
		break;
	}
  return FALSE;
}

LRESULT CALLBACK AATStart(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS lpwp;
	
	switch (message)
	{
		case WM_INITDIALOG:
			if(AATEnabled)
				SendDlgItemMessage(hDlg,IDC_ENABLE,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hDlg,IDC_DISABLE,BM_SETCHECK,BST_CHECKED,0);
			
			SetDlgItemInt(hDlg,IDC_TIME,(int)AATTaskLength,TRUE);
		return TRUE; 

		case WM_COMMAND:
			if (LOWORD(wParam) == IDC_ENABLE) 
			{
				AATEnabled = TRUE;
			}
			if (LOWORD(wParam) == IDC_DISABLE) 
			{
				AATEnabled = FALSE;
			}
		break;

		case WM_WINDOWPOSCHANGED:
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				AATTaskLength = (double)GetDlgItemInt(hDlg,IDC_TIME,NULL,TRUE);
			}
		break;

	}
  return FALSE;
}


LRESULT CALLBACK AATTurn(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	LPWINDOWPOS lpwp;
	int Index = 0;
	
	switch (message)
	{
		case WM_INITDIALOG:
			Index = lParam;

			if(Task[Index].AATType == CIRCLE)
				SendDlgItemMessage(hDlg,IDC_CIRCLE,BM_SETCHECK,BST_CHECKED,0);
			else
				SendDlgItemMessage(hDlg,IDC_SECTOR,BM_SETCHECK,BST_CHECKED,0);
			
			SetDlgItemInt(hDlg,IDC_CIRCLERADIUS,(int)Task[Index].AATCircleRadius,TRUE);
			SetDlgItemInt(hDlg,IDC_SECTORRADIUS,(int)Task[Index].AATSectorRadius ,TRUE);
			SetDlgItemInt(hDlg,IDC_STARTRADIAL, (int)Task[Index].AATStartRadial  ,TRUE);
			SetDlgItemInt(hDlg,IDC_FINISHRADIAL,(int)Task[Index].AATFinishRadial ,TRUE);

		return TRUE; 

		case WM_COMMAND:
			Index = FindIndex(hDlg);
			if (LOWORD(wParam) == IDC_CIRCLE) 
			{
				Task[Index].AATType = CIRCLE;
			}
			if (LOWORD(wParam) == IDC_SECTOR) 
			{
				Task[Index].AATType = SECTOR;
			}
		break;

		case WM_WINDOWPOSCHANGED:
			Index = FindIndex(hDlg);
			lpwp = (LPWINDOWPOS)(lParam);
			if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
			{
				Task[Index].AATCircleRadius = GetDlgItemInt(hDlg,IDC_CIRCLERADIUS,NULL,TRUE);
				Task[Index].AATSectorRadius  = GetDlgItemInt(hDlg,IDC_SECTORRADIUS,NULL,TRUE);
				Task[Index].AATStartRadial  = GetDlgItemInt(hDlg,IDC_STARTRADIAL,NULL,TRUE);
				Task[Index].AATFinishRadial  = GetDlgItemInt(hDlg,IDC_FINISHRADIAL,NULL,TRUE);
			}
		break;

	}
  return FALSE;
}