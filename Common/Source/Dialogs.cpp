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

//   $Id$

*/
#include "stdafx.h"
#include "compatibility.h"

#include "dialogs.h"
#include "resource.h"
#include "utils.h"
#include "externs.h"
#include "Port.h"
#include "McReady.h"
#include "AirfieldDetails.h"
#include "VarioSound.h"
#include "device.h"

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
extern TCHAR szRegistrySafetyAltitudeArrival[];
extern TCHAR szRegistrySafetyAltitudeBreakOff[];
extern TCHAR szRegistrySafetyAltitudeTerrain[];
extern TCHAR szRegistrySafteySpeed[];
extern TCHAR szRegistryFAISector[];
extern TCHAR szRegistrySectorRadius[];
extern TCHAR szRegistryPolarID[];
extern TCHAR szRegistryWayPointFile[];
extern TCHAR szRegistryAirspaceFile[];
extern TCHAR szRegistryAirfieldFile[];
extern TCHAR szRegistryTopologyFile[];
extern TCHAR szRegistryPolarFile[];
extern TCHAR szRegistryTerrainFile[];
extern TCHAR szRegistryAltMode[];
extern TCHAR szRegistryClipAlt[];
extern TCHAR szRegistryAltMargin[];
extern TCHAR szRegistryRegKey[];
extern TCHAR szRegistrySnailTrail[];
extern TCHAR szRegistryDrawTopology[];
extern TCHAR szRegistryDrawTerrain[];
extern TCHAR szRegistryFinalGlideTerrain[];
extern TCHAR szRegistryStartLine[];
extern TCHAR szRegistryStartRadius[];
extern TCHAR szRegistryAirspaceWarning[];
extern TCHAR szRegistryAirspaceBlackOutline[];
extern TCHAR szRegistryWarningTime[];
extern TCHAR szRegistryAcknowledgementTime[];
extern TCHAR szRegistryCircleZoom[];
extern TCHAR szRegistryWindUpdateMode[];
extern TCHAR szRegistryHomeWaypoint[];
extern TCHAR szRegistryPilotName[];
extern TCHAR szRegistryAircraftType[];
extern TCHAR szRegistryAircraftRego[];
extern TCHAR szRegistryNettoSpeed[];


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
        case PROCESSOR_INTEL_386                : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Intel 386")); break;
        case PROCESSOR_INTEL_486                : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Intel 486")); break;
        case PROCESSOR_INTEL_PENTIUM: SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Intel Pentium")); break;
        case PROCESSOR_MIPS_R4000               : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("MIPS R4000")); break;
        case PROCESSOR_HITACHI_SH3      : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Hitachi SH3")); break;
        case PROCESSOR_HITACHI_SH4      : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("Hitachi SH4")); break;
        case PROCESSOR_STRONGARM                : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("StrongARM")); break;
        case PROCESSOR_ARM720                           : SetDlgItemText(hDlg,IDC_PROCESSOR,TEXT("ARM 720")); break;
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


extern int MenuTimeOut;


LRESULT CALLBACK Menu(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
    {
    case WM_INITDIALOG:
      if(DisplayLocked)
        {
          SendDlgItemMessage(hDlg,IDD_LOCK,BM_SETCHECK ,BST_CHECKED, 0);
          SetWindowText(GetDlgItem(hDlg,IDD_LOCK),TEXT("InfoBoxes locked"));
        }
      else
        {
          SendDlgItemMessage(hDlg,IDD_LOCK,BM_SETCHECK ,BST_UNCHECKED, 0);
          SetWindowText(GetDlgItem(hDlg,IDD_LOCK),TEXT("InfoBoxes editable"));
        }

      if(TaskAborted)
        {
          SetWindowText(GetDlgItem(hDlg,IDC_ABORTTASK),TEXT("Resume"));
        }
      else
        {
          SetWindowText(GetDlgItem(hDlg,IDC_ABORTTASK),TEXT("Abort"));
        }

      return TRUE;

    case WM_COMMAND:
      EndDialog(hDlg, LOWORD(wParam));
      MenuTimeOut=10;
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
  TCHAR *NavboxModes[] = {TEXT("Climb"),TEXT("Cruise"),TEXT("Final Glide")};
  int i;
  static int ItemSelected = 0;
  static int navmode = 0;
  static int info_0;
  static int info_1;
  static int info_2;

  switch (message)
    {

    case WM_INITDIALOG:
      for(i=0;i<3;i++)
        SendDlgItemMessage(hDlg,IDC_NAVBOXMODE,CB_ADDSTRING,0,(LPARAM)(LPCSTR)NavboxModes[i]);

      // JMW todo: set mode to current mode by default
      SendDlgItemMessage(hDlg,IDC_NAVBOXMODE,CB_SETCURSEL,(WPARAM)navmode,0);

      // Create a Done button and size it.
      for(i=0;i<NUMSELECTSTRINGS;i++)
        {
          SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_ADDSTRING,0,(LPARAM)(LPCTSTR) Data_Options[i].Description);
        }

      info_0 = CurrentInfoType&0xff;
      info_1 = (CurrentInfoType>>8)&0xff;
      info_2 = (CurrentInfoType>>16)&0xff;

      if (navmode==0) {
        SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_SETCURSEL,(WPARAM)(info_0),0);
      }
      if (navmode==1) {
        SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_SETCURSEL,(WPARAM)(info_1),0);
      }
      if (navmode==2) {
        SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_SETCURSEL,(WPARAM)(info_2),0);
      }

      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK)
        {
          SHSipPreference(hDlg,SIP_FORCEDOWN);

          if (navmode==0) {
            info_0 = SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_GETCURSEL,0,0);
          }
          ItemSelected = info_0;

          if (navmode==1) {
            info_1 = SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_GETCURSEL,0,0);
          }
          ItemSelected += (info_1 <<8);

          if (navmode==2) {
            info_2 = SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_GETCURSEL,0,0);
          }
          ItemSelected += (info_2 << 16);

          EndDialog(hDlg, ItemSelected);
          return TRUE;
        }

      if (HIWORD(wParam) ==  CBN_SELCHANGE)
        {
          int newnavmode = SendDlgItemMessage(hDlg,IDC_NAVBOXMODE,CB_GETCURSEL, 0,0);

          if (navmode != newnavmode) {
            navmode = newnavmode;
            if (navmode==0) {
              SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_SETCURSEL,(WPARAM)(info_0),0);
            }
            if (navmode==1) {
              SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_SETCURSEL,(WPARAM)(info_1),0);
            }
            if (navmode==2) {
              SendDlgItemMessage(hDlg,IDC_NAVBOXOPTION,LB_SETCURSEL,(WPARAM)(info_2),0);
            }
          }

        }
      break;

    }
  return FALSE;
}

int intround(double d) {
  int g = (int)d;
  if (d-g>0.5) {
    return g+1;
  } else {
    return g;
  }
}


LRESULT CALLBACK SetPolar(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  LPWINDOWPOS lpwp;

  TCHAR *Polar[] = {TEXT("Vintage - Ka6"),
                    TEXT("Club Class - ASW19"),
                    TEXT("Standard Class - LS8"),
                    TEXT("15M Class - ASW27"),
                    TEXT("18M Class - LS6C"),
                    TEXT("Open Class ASW22"),
                    TEXT("WinPilot File")};
  int i;
  int Temp;
  int Temp1;
  static ACTIVE = FALSE;
  static TCHAR  szPolarFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofnPolar;
  TCHAR szFile[MAX_PATH];

  switch (message)
    {
    case WM_INITDIALOG:
      ACTIVE = FALSE;

      for(i=0;i<7;i++)
        SendDlgItemMessage(hDlg,IDC_POLAR,CB_ADDSTRING,0,(LPARAM)(LPCSTR)Polar[i]);

      SendDlgItemMessage(hDlg,IDC_POLAR,CB_SETCURSEL,(WPARAM) POLARID,0);
      SetDlgItemInt(hDlg,IDC_MAXSPEED,(int)SAFTEYSPEED,FALSE);
      GetRegistryString(szRegistryPolarFile, szPolarFile, MAX_PATH);
      SetDlgItemText(hDlg,IDC_POLARFILE,szPolarFile);
      ACTIVE = TRUE;

      return TRUE;
      break;

    case WM_COMMAND:
      if (HIWORD(wParam) ==  CBN_SELCHANGE)
        {
          POLARID = SendDlgItemMessage(hDlg,IDC_POLAR,CB_GETCURSEL, 0,0);
          SetToRegistry(szRegistryPolarID,POLARID);
          CalculateNewPolarCoef();
	  return TRUE;
        }
      switch (LOWORD(wParam)) {
      case IDC_POLARFILE:
	if(ACTIVE == TRUE)
	  {
	    if(HIWORD(wParam) == EN_UPDATE)
	      {
		GetDlgItemText(hDlg,IDC_POLARFILE,szFile,MAX_PATH);
		SetRegistryString(szRegistryPolarFile,szFile);
		CalculateNewPolarCoef();
		SetBallast();

	      }
	  }
	break;
      case IDC_BROWSEPOLAR:

	memset( &(ofnPolar), 0, sizeof(ofnPolar));
	ofnPolar.lStructSize  = sizeof(ofnPolar);
	ofnPolar.hwndOwner = hDlg;
	ofnPolar.lpstrFile = szPolarFile;
	ofnPolar.nMaxFile = MAX_PATH;
	ofnPolar.lpstrFilter = TEXT("All Files(*.plr)\0*.plr\0");
	ofnPolar.lpstrTitle = TEXT("Open File");
	ofnPolar.Flags = OFN_EXPLORER;

	if(GetOpenFileName(&ofnPolar))
	  {
	    SetDlgItemText(hDlg,IDC_POLARFILE,szPolarFile);
	  }
	break;
      }
      break; // bug found by sgi

    case WM_WINDOWPOSCHANGED:
      lpwp = (LPWINDOWPOS)(lParam);
	  if (!lpwp)
		break;

      if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
        {
          Temp1 = GetDlgItemInt(hDlg,IDC_MAXSPEED,&Temp,FALSE);
          if(Temp)
            {
              SAFTEYSPEED = Temp1;
              SetToRegistry(szRegistrySafteySpeed,(DWORD)SAFTEYSPEED);
            }
        }
      break;

    }
  return FALSE;
}


LRESULT CALLBACK AudioSettings(HWND hDlg, UINT message,
			       WPARAM wParam, LPARAM lParam)
{
  int Temp;
  int Temp1;
  LPWINDOWPOS lpwp;

  switch (message)
    {
    case WM_INITDIALOG:

      SetDlgItemInt(hDlg,IDC_NETTOSPEED,(int)NettoSpeed,FALSE);

      SendDlgItemMessage(hDlg, IDC_AUDIOSLIDER,
			 TBM_SETRANGE, FALSE, MAKELPARAM(0,100));
      SendDlgItemMessage(hDlg, IDC_AUDIOSLIDER,
			 TBM_SETPOS, TRUE, 100-SoundVolume);

      SendDlgItemMessage(hDlg, IDC_AUDIODEADBAND,
			 TBM_SETRANGE, FALSE, MAKELPARAM(0,40));
      SendDlgItemMessage(hDlg, IDC_AUDIODEADBAND,
			 TBM_SETPOS, TRUE, 40-SoundDeadband);

      if(EnableSoundVario == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_AUDIOVARIO,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_AUDIOVARIO,BM_SETCHECK,BST_UNCHECKED,0);
        }

      if(EnableSoundTask == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_AUDIOEVENTS,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_AUDIOEVENTS,BM_SETCHECK,BST_UNCHECKED,0);
        }

      if(EnableSoundModes == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_AUDIOUI,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_AUDIOUI,BM_SETCHECK,BST_UNCHECKED,0);
        }

      return TRUE;

    case WM_VSCROLL:
      SoundVolume = 100-SendDlgItemMessage(hDlg, IDC_AUDIOSLIDER,
				       TBM_GETPOS, (WPARAM)0, (LPARAM)0);
      SoundDeadband = 40-SendDlgItemMessage(hDlg, IDC_AUDIODEADBAND,
					 TBM_GETPOS, (WPARAM)0, (LPARAM)0);
      VarioSound_SetSoundVolume(SoundVolume);
      VarioSound_SetVdead(SoundDeadband);
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
        {
        case IDC_AUDIOVARIO:
          EnableSoundVario = !EnableSoundVario;
	  VarioSound_EnableSound(EnableSoundVario); // remove type case RB
          return TRUE;
        case IDC_AUDIOEVENTS:
          EnableSoundTask = !EnableSoundTask;
          return TRUE;
        case IDC_AUDIOUI:
          EnableSoundModes = !EnableSoundModes;
          return TRUE;

        }
      break;

    case WM_WINDOWPOSCHANGED:
      lpwp = (LPWINDOWPOS)(lParam);
	  if (!lpwp)
		break;

      if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
        {
          Temp1 = GetDlgItemInt(hDlg,IDC_NETTOSPEED,&Temp,FALSE);
          if(Temp)
            {
              NettoSpeed = Temp1;
              SetToRegistry(szRegistryNettoSpeed,(DWORD)NettoSpeed);
            }
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

      if(EnableTopology == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_DRAWTOPOLOGY,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_DRAWTOPOLOGY,BM_SETCHECK,BST_UNCHECKED,0);
        }

      if(EnableTerrain == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_DRAWTERRAIN,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_DRAWTERRAIN,BM_SETCHECK,BST_UNCHECKED,0);
        }

      if(CircleZoom == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_CIRCLEZOOM,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_CIRCLEZOOM,BM_SETCHECK,BST_UNCHECKED,0);
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

        case TRACKCIRCLE :
          SendDlgItemMessage(hDlg,IDC_TRACKCIRCLE,BM_SETCHECK,BST_CHECKED,0);
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

        case 5 :
          SendDlgItemMessage(hDlg,IDC_WAYPOINTNAMEIFINTASK,BM_SETCHECK,BST_CHECKED,0);
          break;

        }

      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
        {
        case IDC_CIRCLEZOOM:
          CircleZoom = !CircleZoom;
          SetToRegistry(szRegistryCircleZoom,CircleZoom);
          return TRUE;

        case IDC_SNAIL:
          TrailActive = !TrailActive;
          SetToRegistry(szRegistrySnailTrail,TrailActive);
          return TRUE;

        case IDC_DRAWTOPOLOGY:
          EnableTopology = !EnableTopology;
          SetToRegistry(szRegistryDrawTopology,EnableTopology);
          return TRUE;

        case IDC_DRAWTERRAIN:
          EnableTerrain = !EnableTerrain;
          SetToRegistry(szRegistryDrawTerrain,EnableTerrain);
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

        case IDC_TRACKCIRCLE:
          SetToRegistry(szRegistryDisplayUpValue,TRACKCIRCLE);
          DisplayOrientation = TRACKCIRCLE;
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

        case IDC_WAYPOINTNAMEIFINTASK:
          SetToRegistry(szRegistryDisplayText,5);
          DisplayTextType = DISPLAYNAMEIFINTASK;
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
      SetDlgItemInt(hDlg,IDC_ACKNOW,(int)AcknowledgementTime,TRUE);

      return TRUE;

    case WM_WINDOWPOSCHANGED:
      lpwp = (LPWINDOWPOS)(lParam);
      if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
        {
          WarningTime  = GetDlgItemInt(hDlg,IDC_EDIT,NULL,TRUE);
          SetToRegistry(szRegistryWarningTime,(DWORD)WarningTime);
          AcknowledgementTime  = GetDlgItemInt(hDlg,IDC_ACKNOW,NULL,TRUE);
          SetToRegistry(szRegistryAcknowledgementTime,
                        (DWORD)AcknowledgementTime);
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

          SetToRegistry(szRegistryAirspaceWarning,(DWORD)AIRSPACEWARNINGS);

          WarningTime  = GetDlgItemInt(hDlg,IDC_EDIT,NULL,TRUE);
          SetToRegistry(szRegistryWarningTime,(DWORD)WarningTime);

          AcknowledgementTime  = GetDlgItemInt(hDlg,IDC_ACKNOW,NULL,TRUE);
          SetToRegistry(szRegistryAcknowledgementTime,
                        (DWORD)AcknowledgementTime);

        }
      break;
    }
  return FALSE;
}

void settaskUpdateControls(HWND hDlg, int TaskSize){

  long Selection;


  AATEnabled = TaskSize >=3;

  EnableWindow(GetDlgItem(hDlg, IDC_AAT), AATEnabled);

  EnableWindow(GetDlgItem(hDlg, IDC_DECLARE), TaskSize >= 2);

  EnableWindow(GetDlgItem(hDlg, IDC_SAVE), TaskSize >= 1);

  Selection = SendDlgItemMessage(hDlg,IDC_WAYPOINTS,LB_GETCURSEL,0,0L);
  EnableWindow(GetDlgItem(hDlg, IDC_WAYPOINTDETAILS), NumberOfWayPoints > 0 && Selection != LB_ERR);
  EnableWindow(GetDlgItem(hDlg, IDC_ADD), NumberOfWayPoints > 0 && Selection != LB_ERR);

  Selection = SendDlgItemMessage(hDlg,IDC_TASK,LB_GETCURSEL,0,0L);
  EnableWindow(GetDlgItem(hDlg, IDC_REMOVE), TaskSize > 0 && Selection != LB_ERR);

}


LRESULT CALLBACK SetTask(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  unsigned int i;
  long Selection;
  long InsertPoint;
  static int TaskSize = 0;
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofn;
  SHINITDLGINFO shidi;


  switch (message)
    {
    case WM_INITDIALOG:
      shidi.dwMask = SHIDIM_FLAGS;
      shidi.dwFlags = SHIDIF_DONEBUTTON | SHIDIF_SIPDOWN | SHIDIF_FULLSCREENNOMENUBAR;
      shidi.hDlg = hDlg;
      SHInitDialog(&shidi);

      TaskSize = 0;

      // JMW TODO: optinally sort by range...

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

      settaskUpdateControls(hDlg, TaskSize);

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

          settaskUpdateControls(hDlg, TaskSize);

          break;

        case IDC_REMOVE:
          Selection = SendDlgItemMessage(hDlg,IDC_TASK,LB_GETCURSEL,0,0L);
          if(Selection != LB_ERR)
            {
              SendDlgItemMessage(hDlg,IDC_TASK,LB_DELETESTRING,Selection,0L);
              TaskSize --;
              ReadNewTask(hDlg);
              if (TaskSize > 0){
                if (Selection >= TaskSize)
                  Selection = TaskSize-1;
                SendDlgItemMessage(hDlg, IDC_TASK, LB_SETCURSEL, (LPARAM)(UINT)Selection, 0L);
            }
            }
          settaskUpdateControls(hDlg, TaskSize);
          break;

        case IDC_WAYPOINTDETAILS:
          SelectedWaypoint = SendDlgItemMessage(hDlg,IDC_WAYPOINTS,LB_GETCURSEL,0,0L);
	  PopupWaypointDetails();
	  ReadNewTask(hDlg); // in case user added/removed
	  break;
        case IDC_LOAD:
          memset( &(ofn), 0, sizeof(ofn));
          ofn.lStructSize       = sizeof(ofn);
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
              if(Task[i].Index >=0)     TaskSize ++;
            }

          settaskUpdateControls(hDlg, TaskSize);
          break;

        case IDC_SAVE:
          memset( &(ofn), 0, sizeof(ofn));
          ofn.lStructSize       = sizeof(ofn);
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

        case IDC_TASK:
        case IDC_WAYPOINTS:
          settaskUpdateControls(hDlg, TaskSize);
        break;


        case IDC_DECLARE:{

          WCHAR PilotName[64];
          WCHAR AircraftType[32];
          WCHAR AircraftRego[32];

          GetRegistryString(szRegistryPilotName, PilotName, 64);
          GetRegistryString(szRegistryAircraftType, AircraftType, 32);
          GetRegistryString(szRegistryAircraftRego, AircraftRego, 32);


          if (devIsLogger(devA())){

            if(MessageBox(hWndMapWindow, TEXT("Declare Task?"), devA()->Name, MB_YESNO| MB_ICONQUESTION) == IDYES){

              devDeclBegin(devA(), PilotName, AircraftType, AircraftRego);
              for(i=0;i<MAXTASKPOINTS;i++)
                {
                  if(Task[i].Index == -1) break;
                  devDeclAddWayPoint(devA(), &WayPointList[Task[i].Index]);
                }
              if (devDeclEnd(devA()))
                MessageBox(hWndMapWindow, TEXT("Task Declared!"), devA()->Name, MB_OK| MB_ICONINFORMATION);
              else
                MessageBox(hWndMapWindow, TEXT("Error occure,\r\nTask NOT Declared!"), devA()->Name, MB_OK| MB_ICONERROR);

            }
          }

          if (devIsLogger(devB())){

            if(MessageBox(hWndMapWindow, TEXT("Declare Task?"), devB()->Name, MB_YESNO| MB_ICONQUESTION) == IDYES){

              devDeclBegin(devB(), PilotName, AircraftType, AircraftRego);
              for(i=0;i<MAXTASKPOINTS;i++)
                {
                  if(Task[i].Index == -1) break;
                  devDeclAddWayPoint(devB(), &WayPointList[Task[i].Index]);
                }
              if (devDeclEnd(devB()))
                MessageBox(hWndMapWindow, TEXT("Task Declared!"), devB()->Name, MB_OK| MB_ICONINFORMATION);
              else
                MessageBox(hWndMapWindow, TEXT("Error occure,\r\nTask NOT Declared!"), devB()->Name, MB_OK| MB_ICONERROR);

            }
          }
        }
        break;

        }

      break;
    }
  return FALSE;
}


void RefreshTaskWaypoint(int i) {
  if(i==0)
    {
      Task[i].Leg = 0;
      Task[i].InBound = 0;
    }
  else
    {
      Task[i].Leg = Distance(WayPointList[Task[i].Index].Lattitude,
                             WayPointList[Task[i].Index].Longditude,
                             WayPointList[Task[i-1].Index].Lattitude,
                             WayPointList[Task[i-1].Index].Longditude);
      Task[i].InBound = Bearing(WayPointList[Task[i-1].Index].Lattitude,   WayPointList[Task[i-1].Index].Longditude,
                                WayPointList[Task[i].Index].Lattitude, WayPointList[Task[i].Index].Longditude);
      Task[i-1].OutBound = Task[i].InBound;
      Task[i-1].Bisector = BiSector(Task[i-1].InBound,Task[i-1].OutBound);
    }
}


void ReadNewTask(HWND hDlg)
{
  int i;
  int TaskSize;
  int WayPointIndex;
  double TaskLength = 0;
  TCHAR szTaskLength[10];
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

          RefreshTaskWaypoint(i);

          // JMW TODO: do this for next and previous waypoint also

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
      SetDlgItemInt(hDlg,IDC_SALTARRIVAL,(int)SAFETYALTITUDEARRIVAL,FALSE);
      SetDlgItemInt(hDlg,IDC_SALTBREAKOFF,(int)SAFETYALTITUDEBREAKOFF,FALSE);
      SetDlgItemInt(hDlg,IDC_SALTTERRAIN,(int)SAFETYALTITUDETERRAIN,FALSE);

      if(FinalGlideTerrain == TRUE)
        {
          SendDlgItemMessage(hDlg,IDC_FINALGLIDETERRAIN,BM_SETCHECK,BST_CHECKED,0);
        }
      else
        {
          SendDlgItemMessage(hDlg,IDC_FINALGLIDETERRAIN,BM_SETCHECK,BST_UNCHECKED,0);
        }

      return TRUE;

    case WM_WINDOWPOSCHANGED:
      lpwp = (LPWINDOWPOS)(lParam);
      if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
        {
          Temp1 = GetDlgItemInt(hDlg,IDC_SALTARRIVAL,&Temp,FALSE);
          if(Temp)
            {
              SAFETYALTITUDEARRIVAL = (double)Temp1;
              SetToRegistry(szRegistrySafetyAltitudeArrival,(DWORD)SAFETYALTITUDEARRIVAL);
            }
          Temp1 = GetDlgItemInt(hDlg,IDC_SALTBREAKOFF,&Temp,FALSE);
          if(Temp)
            {
              SAFETYALTITUDEBREAKOFF = (double)Temp1;
              SetToRegistry(szRegistrySafetyAltitudeBreakOff,(DWORD)SAFETYALTITUDEBREAKOFF);
            }
          Temp1 = GetDlgItemInt(hDlg,IDC_SALTTERRAIN,&Temp,FALSE);
          if(Temp)
            {
              SAFETYALTITUDETERRAIN = (double)Temp1;
              SetToRegistry(szRegistrySafetyAltitudeTerrain,(DWORD)SAFETYALTITUDETERRAIN);
            }
        }
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDC_FINALGLIDETERRAIN:
	  FinalGlideTerrain = !FinalGlideTerrain;
	  SetToRegistry(szRegistryFinalGlideTerrain,FinalGlideTerrain);
	  return TRUE;
	}
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
      logfont.lfHeight = 35;
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

	  double lastBugs = BUGS;
	  double lastBallast = BALLAST;

          T1 = GetDlgItemInt(hDlg,IDC_BUGS,&T2,FALSE);
          if(T2)
            {

              BUGS = (double)T1;
              BUGS /=100;
              if(BUGS<0.5) BUGS = 0.5;

              if (lastBugs != BUGS){
                devPutBugs(devA(), BUGS);
                devPutBugs(devB(), BUGS);
              }
          }
          T1 = GetDlgItemInt(hDlg,IDC_BALLAST,&T2,FALSE);
          if(T2)
            {
              BALLAST = (double)T1;
              BALLAST /=100;
	      SetBallast();
              if (lastBallast != BALLAST){
                devPutBallast(devA(), BALLAST);
                devPutBallast(devB(), BALLAST);
              }
            }

          EndDialog(hDlg, LOWORD(wParam));
          DeleteObject(hFont);
          return TRUE;
        }
      if (LOWORD(wParam) == IDC_BUGSUP)
        {
          T1 = GetDlgItemInt(hDlg,IDC_BUGS,&T2,FALSE);
          T1 += 5; if(T1>100) T1 = 100;
          SetDlgItemInt(hDlg,IDC_BUGS,(int)T1,FALSE);
        }
      if (LOWORD(wParam) == IDC_BUGSDOWN)
        {
          T1 = GetDlgItemInt(hDlg,IDC_BUGS,&T2,FALSE);
          T1 -= 5; if(T1<50) T1 = 50;
          SetDlgItemInt(hDlg,IDC_BUGS,(int)T1,FALSE);
        }
      if (LOWORD(wParam) == IDC_BALLASTUP)
        {
          T1 = GetDlgItemInt(hDlg,IDC_BALLAST,&T2,FALSE);
          T1 += 10; if(T1>100) T1 = 100;
          SetDlgItemInt(hDlg,IDC_BALLAST,(int)T1,FALSE);
        }
      if (LOWORD(wParam) == IDC_BALLASTDOWN)
        {
          T1 = GetDlgItemInt(hDlg,IDC_BALLAST,&T2,FALSE);
          T1 -= 10; if(T1<0) T1 = 0;
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
  static TCHAR  szAirspaceFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofnAirspace;
  static TCHAR  szWaypointFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofnWaypoint;
  static TCHAR  szTerrainFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofnTerrain;
  static TCHAR  szAirfieldFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofnAirfield;
  static TCHAR  szTopologyFile[MAX_PATH] = TEXT("\0");
  static OPENFILENAME           ofnTopology;

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
      GetRegistryString(szRegistryAirfieldFile, szAirfieldFile, MAX_PATH);
      SetDlgItemText(hDlg,IDC_AIRFIELDSFILE,szAirfieldFile);
      GetRegistryString(szRegistryTopologyFile, szTopologyFile, MAX_PATH);
      SetDlgItemText(hDlg,IDC_TOPOLOGYFILE,szTopologyFile);

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

        case IDC_AIRFIELDSFILE:
          if(ACTIVE == TRUE)
            {
              if(HIWORD(wParam) == EN_UPDATE)
                {
                  AIRFIELDFILECHANGED = TRUE;
                  GetDlgItemText(hDlg,IDC_AIRFIELDSFILE,szFile,MAX_PATH);
                  SetRegistryString(szRegistryAirfieldFile,szFile);
                }
            }
          break;

        case IDC_TOPOLOGYFILE:
          if(ACTIVE == TRUE)
            {
              if(HIWORD(wParam) == EN_UPDATE)
                {

				  TOPOLOGYFILECHANGED = TRUE;
                  GetDlgItemText(hDlg,IDC_TOPOLOGYFILE,szFile,MAX_PATH);
                  SetRegistryString(szRegistryTopologyFile,szFile);
                }
            }
          break;

        case IDC_BROWSEAIRSPACE:
          memset( &(ofnAirspace), 0, sizeof(ofnAirspace));
          ofnAirspace.lStructSize       = sizeof(ofnAirspace);
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
          ofnTerrain.lStructSize        = sizeof(ofnTerrain);
          ofnTerrain.hwndOwner = hDlg;
          ofnTerrain.lpstrFile = szTerrainFile;
          ofnTerrain.nMaxFile = MAX_PATH;
          ofnTerrain.lpstrFilter = TEXT("All Files(*.dat)\0*.dat\0");
          ofnTerrain.lpstrTitle = TEXT("Open File");
          ofnTerrain.Flags = OFN_EXPLORER;

          if(GetOpenFileName(&ofnTerrain))
            {
              SetDlgItemText(hDlg,IDC_TERRAINFILE,szTerrainFile);
            }
          break;

        case IDC_BROWSEWAYPOINT:
          memset( &(ofnWaypoint), 0, sizeof(ofnWaypoint));
          ofnWaypoint.lStructSize       = sizeof(ofnWaypoint);
          ofnWaypoint.hwndOwner = hDlg;
          ofnWaypoint.lpstrFile = szWaypointFile;
          ofnWaypoint.nMaxFile = MAX_PATH;
          ofnWaypoint.lpstrFilter = TEXT("All Files(*.dat)\0*.dat\0");
          ofnWaypoint.lpstrTitle = TEXT("Open File");
          ofnWaypoint.Flags = OFN_EXPLORER;

          if(GetOpenFileName(&ofnWaypoint))
            {
              SetDlgItemText(hDlg,IDC_WAYPOINTSFILE,szWaypointFile);
            }
          break;

        case IDC_BROWSEAIRFIELDS:

          memset( &(ofnAirfield), 0, sizeof(ofnAirfield));
          ofnAirfield.lStructSize       = sizeof(ofnAirfield);
          ofnAirfield.hwndOwner = hDlg;
          ofnAirfield.lpstrFile = szAirfieldFile;
          ofnAirfield.nMaxFile = MAX_PATH;
          ofnAirfield.lpstrFilter = TEXT("All Files(*.txt)\0*.txt\0");
          ofnAirfield.lpstrTitle = TEXT("Open File");
          ofnAirfield.Flags = OFN_EXPLORER;

          if(GetOpenFileName(&ofnAirfield))
            {
              SetDlgItemText(hDlg,IDC_AIRFIELDSFILE,szAirfieldFile);
            }
          break;

        case IDC_BROWSETOPOLOGY:

          memset( &(ofnTopology), 0, sizeof(ofnTopology));
          ofnTopology.lStructSize       = sizeof(ofnTopology);
          ofnTopology.hwndOwner = hDlg;
          ofnTopology.lpstrFile = szTopologyFile;
          ofnTopology.nMaxFile = MAX_PATH;
          ofnTopology.lpstrFilter = TEXT("All Files(*.tpl)\0*.tpl\0");
          ofnTopology.lpstrTitle = TEXT("Open File");
          ofnTopology.Flags = OFN_EXPLORER;

          if(GetOpenFileName(&ofnTopology))
            {
              SetDlgItemText(hDlg,IDC_TOPOLOGYFILE,szTopologyFile);
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
	  //          ClipAltitude  = LOWORD(SendDlgItemMessage(hDlg,IDC_CLIPSPIN,UDM_GETPOS ,0,0L));
	  //          SetToRegistry(szRegistryClipAlt,ClipAltitude);

	  //          AltWarningMargin = LOWORD(SendDlgItemMessage(hDlg,IDC_MARGINSPIN,UDM_GETPOS ,0,0L));
	  //          SetToRegistry(szRegistryAltMargin,AltWarningMargin);

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
  double alt;
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
      SendDlgItemMessage(hDlg,IDC_PALTITUDE,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));

      SendDlgItemMessage(hDlg,IDC_UP,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_DOWN,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC1,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC2,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC3,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
      SendDlgItemMessage(hDlg,IDC_STATIC4,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));

      SetDlgItemInt(hDlg,IDC_QNH,(int)QNH,FALSE);
      INHg = (int)QNH;
      INHg = INHg /1013.2;
      INHg = INHg*29.91;
      wsprintf(Temp,TEXT("%2.2f"),INHg );
      SetDlgItemText(hDlg,IDC_INHG,Temp);

      LockFlightData();
      alt =   GPS_INFO.BaroAltitude*ALTITUDEMODIFY;
      UnlockFlightData();
      wsprintf(Temp,TEXT("%2.0f"),alt);
      SetDlgItemText(hDlg,IDC_PALTITUDE,Temp);

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

          LockFlightData();
          alt =   GPS_INFO.BaroAltitude*ALTITUDEMODIFY;
          UnlockFlightData();
          wsprintf(Temp,TEXT("%2.0f"),alt);
          SetDlgItemText(hDlg,IDC_PALTITUDE,Temp);

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
int NewBrush;

LRESULT CALLBACK MapColour(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
    {
    case WM_INITDIALOG:
      if(bAirspaceBlackOutline) {
        SendDlgItemMessage(hDlg,IDC_BLACKOUTLINE,BM_SETCHECK,BST_CHECKED,0);
      } else {
        SendDlgItemMessage(hDlg,IDC_BLACKOUTLINE,BM_SETCHECK,BST_UNCHECKED,0);
      }

      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
        {
        case IDOK:
          EndDialog(hDlg, TRUE);
          return TRUE;

        case IDC_CLASSA:
          NewColor = iAirspaceColour[CLASSA];
          NewBrush = iAirspaceBrush[CLASSA];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[CLASSA] = NewBrush;
          iAirspaceColour[CLASSA] = NewColor;
          SetRegistryColour(CLASSA,NewColor);
          SetRegistryBrush(CLASSA, NewBrush);
          return TRUE;

        case IDC_CLASSB:
          NewBrush = iAirspaceBrush[CLASSB];
          NewColor = iAirspaceColour[CLASSB];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[CLASSB] = NewBrush;
          iAirspaceColour[CLASSB] = NewColor;
          SetRegistryColour(CLASSB,NewColor);
          SetRegistryBrush(CLASSB,NewBrush);
          return TRUE;

        case IDC_CLASSC:
          NewBrush = iAirspaceBrush[CLASSC];
          NewColor = iAirspaceColour[CLASSC];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[CLASSC] = NewBrush;
          iAirspaceColour[CLASSC] = NewColor;
          SetRegistryColour(CLASSC,NewColor);
          SetRegistryBrush(CLASSC,NewBrush);
          return TRUE;

        case IDC_CLASSD:
          NewBrush = iAirspaceBrush[CLASSD];
          NewColor = iAirspaceColour[CLASSD];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[CLASSD] = NewBrush;
          iAirspaceColour[CLASSD] = NewColor;
          SetRegistryColour(CLASSD,NewColor);
          SetRegistryBrush(CLASSD,NewBrush);
          return TRUE;

        case IDC_PROHIBITED:
          NewBrush = iAirspaceBrush[PROHIBITED];
          NewColor = iAirspaceColour[PROHIBITED];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[PROHIBITED] = NewBrush;
          iAirspaceColour[PROHIBITED] = NewColor;
          SetRegistryColour(PROHIBITED,NewColor);
          SetRegistryBrush(PROHIBITED,NewBrush);
          return TRUE;

        case IDC_DANGER:
          NewBrush = iAirspaceBrush[DANGER];
          NewColor = iAirspaceColour[DANGER];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[DANGER] = NewBrush;
          iAirspaceColour[DANGER] = NewColor;
          SetRegistryColour(DANGER,NewColor);
          SetRegistryBrush(DANGER,NewBrush);
          return TRUE;

        case IDC_RESTRICTED:
          NewBrush = iAirspaceBrush[RESTRICT];
          NewColor = iAirspaceColour[RESTRICT];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[RESTRICT] = NewBrush;
          iAirspaceColour[RESTRICT] = NewColor;
          SetRegistryColour(RESTRICT,NewColor);
          SetRegistryBrush(RESTRICT,NewBrush);
          return TRUE;

        case IDC_CTR:
          NewBrush = iAirspaceBrush[CTR];
          NewColor = iAirspaceColour[CTR];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[CTR] = NewBrush;
          iAirspaceColour[CTR] = NewColor;
          SetRegistryColour(CTR,NewColor);
          SetRegistryBrush(CTR,NewBrush);
          return TRUE;

        case IDC_NOGLIDER:
          NewBrush = iAirspaceBrush[NOGLIDER];
          NewColor = iAirspaceColour[NOGLIDER];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[NOGLIDER] = NewBrush;
          iAirspaceColour[NOGLIDER] = NewColor;
          SetRegistryColour(NOGLIDER,NewColor);
          SetRegistryBrush(NOGLIDER,NewBrush);
          return TRUE;

        case IDC_WAVE:
          NewBrush = iAirspaceBrush[WAVE];
          NewColor = iAirspaceColour[WAVE];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[WAVE] = NewBrush;
          iAirspaceColour[WAVE] = NewColor;
          SetRegistryColour(WAVE,NewColor);
          SetRegistryBrush(WAVE,NewBrush);
          return TRUE;

        case IDC_OTHER:
          NewBrush = iAirspaceBrush[OTHER];
          NewColor = iAirspaceColour[OTHER];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[OTHER] = NewBrush;
          iAirspaceColour[OTHER] = NewColor;
          SetRegistryColour(OTHER,NewColor);
          SetRegistryBrush(OTHER,NewBrush);
          return TRUE;

        case IDC_AAT:
          NewBrush = iAirspaceBrush[AATASK];
          NewColor = iAirspaceColour[AATASK];
          DialogBox(hInst, (LPCTSTR)IDD_COLOURSEL, hDlg, (DLGPROC)ColourSelect);
          iAirspaceBrush[AATASK] = NewBrush;
          iAirspaceColour[AATASK] = NewColor;
          SetRegistryColour(AATASK,NewColor);
          SetRegistryBrush(AATASK,NewBrush);
          return TRUE;

        case IDC_BLACKOUTLINE:
          bAirspaceBlackOutline = !bAirspaceBlackOutline;
          SetToRegistry(szRegistryAirspaceBlackOutline,bAirspaceBlackOutline);
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
      Temp = Temp | WS_EX_CLIENTEDGE;
      SetWindowLong(GetDlgItem(hDlg,IDC_BM1+NewColor),GWL_STYLE,Temp);

      Temp = GetWindowLong(GetDlgItem(hDlg,IDC_BM1+NewColor),GWL_EXSTYLE);
      Temp = Temp | WS_EX_CLIENTEDGE;
      SetWindowLong(GetDlgItem(hDlg,IDC_BM1+NewColor),GWL_EXSTYLE,Temp);

      Temp = GetWindowLong(GetDlgItem(hDlg,IDC_BM18+NewBrush),GWL_STYLE);
      Temp = Temp | WS_BORDER;
      SetWindowLong(GetDlgItem(hDlg,IDC_BM18+NewBrush),GWL_STYLE,Temp);

      Temp = GetWindowLong(GetDlgItem(hDlg,IDC_BM18+NewBrush),GWL_EXSTYLE);
      Temp = Temp | WS_EX_CLIENTEDGE;
      SetWindowLong(GetDlgItem(hDlg,IDC_BM18+NewBrush),GWL_EXSTYLE,Temp);

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

          /*        JMW    No longer required
        case IDC_BM17:
          NewColor = 16;
          EndDialog(hDlg, TRUE);
          return TRUE;
          */

          // new pattern bits
        case IDC_BM18:
          NewBrush = 0;
          EndDialog(hDlg, TRUE);
          return TRUE;

        case IDC_BM19:
          NewBrush = 1;
          EndDialog(hDlg, TRUE);
          return TRUE;

        case IDC_BM20:
          NewBrush = 2;
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
  DWORD dwSpeed[] = {1200,2400,4800,9600,19200,38400,57600,115200};
  int i;
  DWORD dwPortIndex = 0;
  DWORD dwSpeedIndex = 2;
  int Index=0;
  TCHAR DeviceName[DEVNAMESIZE+1];

  switch (message)
    {
    case WM_INITDIALOG:
      SendDlgItemMessage(hDlg, IDC_DEVICE, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)TEXT("Generic GPS"));
      for (i=0; i<DeviceRegisterCount; i++){
        decRegisterGetName(i, DeviceName);
        SendDlgItemMessage(hDlg, IDC_DEVICE, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)DeviceName);
        if (devA() != NULL){
          if (_tcscmp(DeviceName, devA()->Name) == 0)
            Index = i+1;
        }
      }
      SendDlgItemMessage(hDlg, IDC_DEVICE, CB_SETCURSEL, (WPARAM) Index, 0);

      for(i=0;i<10;i++)
        SendDlgItemMessage(hDlg,IDC_COMMPORT,CB_ADDSTRING,0,(LPARAM)(LPCSTR)COMMPort[i]);
      for(i=0;i<8;i++)
        SendDlgItemMessage(hDlg,IDC_PORTSPEED,CB_ADDSTRING,0,(LPARAM)(LPCSTR)Speed[i]);

      ReadPort1Settings(&dwPortIndex,&dwSpeedIndex);

      SendDlgItemMessage(hDlg,IDC_COMMPORT,CB_SETCURSEL,(WPARAM) dwPortIndex,0);
      SendDlgItemMessage(hDlg,IDC_PORTSPEED,CB_SETCURSEL,(WPARAM) dwSpeedIndex,0);

      for(i=0;i<10;i++)
        SendDlgItemMessage(hDlg,IDC_COMMPORT2,CB_ADDSTRING,0,(LPARAM)(LPCSTR)COMMPort[i]);
      for(i=0;i<8;i++)
        SendDlgItemMessage(hDlg,IDC_PORTSPEED2,CB_ADDSTRING,0,(LPARAM)(LPCSTR)Speed[i]);
      ReadPort2Settings(&dwPortIndex,&dwSpeedIndex);

      SendDlgItemMessage(hDlg,IDC_COMMPORT2,CB_SETCURSEL,(WPARAM) dwPortIndex,0);
      SendDlgItemMessage(hDlg,IDC_PORTSPEED2,CB_SETCURSEL,(WPARAM) dwSpeedIndex,0);

      return TRUE;

    case WM_COMMAND:
      if (HIWORD(wParam) ==  CBN_SELCHANGE)
        {
          DWORD x;
          COMPORTCHANGED = TRUE;
          x = SendDlgItemMessage(hDlg, IDC_DEVICE, CB_GETCURSEL, 0, 0);
          decRegisterGetName(x-1, DeviceName);
          WriteDeviceSettings(0, DeviceName);
          dwPortIndex = SendDlgItemMessage(hDlg,IDC_COMMPORT,CB_GETCURSEL, 0,0);
          dwSpeedIndex = SendDlgItemMessage(hDlg,IDC_PORTSPEED,CB_GETCURSEL, 0,0);
          WritePort1Settings(dwPortIndex,dwSpeedIndex);

          dwPortIndex = SendDlgItemMessage(hDlg,IDC_COMMPORT2,CB_GETCURSEL, 0,0);
          dwSpeedIndex = SendDlgItemMessage(hDlg,IDC_PORTSPEED2,CB_GETCURSEL, 0,0);
          WritePort2Settings(dwPortIndex,dwSpeedIndex);
        }
      break;
    }
  return FALSE;
}

static OPENFILENAME             profileofn;

LRESULT CALLBACK LoadProfile(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");

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
          profileofn.lStructSize        = sizeof(profileofn);
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
  static TCHAR  szFile[MAX_PATH] = TEXT("\0");

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
          profileofn.lStructSize        = sizeof(profileofn);
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



#define NUMPAGES 16
static const TCHAR *szSettingsTab[] =
{
  TEXT("About"),
  TEXT("Register"),
  TEXT("Airspace display"),
  TEXT("Airspace colours"),
  TEXT("Airspace warnings"),
  TEXT("COMM"),
  TEXT("Display"),
  TEXT("Files"),
  TEXT("Final glide"),
  TEXT("Polar"),
  TEXT("Profile load"),
  TEXT("Profile save"),
  TEXT("Task"),
  TEXT("Units"),
  TEXT("Logger details"),
  TEXT("Audio"),
};


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
          SendDlgItemMessage(hDlg,IDC_SETTINGS,CB_ADDSTRING,0,
			     (LPARAM)(LPCSTR)szSettingsTab[i]);
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
      hTabPage[14] = CreateDialog(hInst, (LPCTSTR)IDD_LOGGERDETAILS, hDlg, (DLGPROC)LoggerDetails);

      hTabPage[15] = CreateDialog(hInst, (LPCTSTR)IDD_AUDIO, hDlg, (DLGPROC)AudioSettings);

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



//////////////////
extern void DrawJPG(HDC hdc, RECT rc);
#include "VOIMAGE.h"

LRESULT CALLBACK WaypointDetails(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
  TCHAR Temp[2048];
  static CVOImage jpgimage1;
  static CVOImage jpgimage2;
  static HDC hdcScreen;
  TCHAR path_modis[100];
  TCHAR path_fname1[] = TEXT("\\Program Files\\omap\\wms.cgi.jpeg");
  TCHAR path_fname2[] = TEXT("\\Program Files\\omap\\ersa-benalla.jpg");
  static int page = 0;
  PAINTSTRUCT ps;
  HDC hdc;
  RECT rc;

  switch (message)
    {
    case WM_INITDIALOG:

      hdcScreen = GetDC(hDlg);

      wsprintf(path_modis,TEXT("\\SD Card\\XCSoar\\MODIS\\modis-%03d.jpg"),
	       SelectedWaypoint+1);

      jpgimage1.Load (hdcScreen ,path_modis );
      jpgimage2.Load (hdcScreen ,path_fname2 );
      page = 0;

      wsprintf(Temp,TEXT("%s\n%s"),
               WayPointList[SelectedWaypoint].Name,
               WayPointList[SelectedWaypoint].Comment);

      SetDlgItemText(hDlg,IDC_WAYPOINTDETAILSTEXT, Temp);

	Temp[0]= 0;

	/*
        if( (WayPointList[SelectedWaypoint].Flags & HOME) == HOME) {
	  wcscat(Temp,TEXT(" Home"));
        }
        if ((WayPointList[SelectedWaypoint].Flags & AIRPORT) == AIRPORT) {
	  wcscat(Temp,TEXT(" Airport"));
	}
        if ((WayPointList[SelectedWaypoint].Flags & LANDPOINT) == LANDPOINT) {
	  wcscat(Temp,TEXT(" Landpoint"));
	}
        if ((WayPointList[SelectedWaypoint].Flags & TURNPOINT) == TURNPOINT) {
	  wcscat(Temp,TEXT(" Turnpoint"));
	}
        if ((WayPointList[SelectedWaypoint].Flags & START) == START) {
	  wcscat(Temp,TEXT(" Start"));
	}
        if ((WayPointList[SelectedWaypoint].Flags & RESTRICTED) == RESTRICTED) {
	  wcscat(Temp,TEXT(" Restricted"));
	}
        if ((WayPointList[SelectedWaypoint].Flags & WAYPOINTFLAG) == WAYPOINTFLAG) {
	  wcscat(Temp,TEXT(" Waypoint"));
	}
	wcscat(Temp,TEXT("\r\n"));
        if (WayPointList[SelectedWaypoint].Reachable) {
	  wcscat(Temp,TEXT(" Reachable"));
        } else {
	  wcscat(Temp,TEXT(" Unreachable"));
        }
	wcscat(Temp,TEXT("\r\n"));
	*/
	wsprintf(Temp,TEXT("Longitude %-3.4f\r\nLatitude %-3.4f\r\nElevation %5.0f\r\n"),
		 WayPointList[SelectedWaypoint].Longditude,
		 WayPointList[SelectedWaypoint].Lattitude,
		 WayPointList[SelectedWaypoint].Altitude*ALTITUDEMODIFY

		 );

	wcscat(Temp,TEXT("\r\n"));

	if (WayPointList[SelectedWaypoint].Details) {
	  wcscat(Temp,WayPointList[SelectedWaypoint].Details);
	}

        SetDlgItemText(hDlg,IDC_WDTEXT, Temp);

      return TRUE;

    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK)
        {
          ::ReleaseDC(hDlg, hdcScreen);
          EndDialog(hDlg, LOWORD(wParam));
          return TRUE;
        }
      if (LOWORD(wParam) == IDC_WDGOTO) {

        FlyDirectTo(SelectedWaypoint);

        ::ReleaseDC(hDlg, hdcScreen);
        EndDialog(hDlg, LOWORD(wParam));
        break;
      }
      if (LOWORD(wParam) == IDC_WDREPLACE) {

        ReplaceWaypoint(SelectedWaypoint);

        ::ReleaseDC(hDlg, hdcScreen);
        EndDialog(hDlg, LOWORD(wParam));
        break;
      }
      if (LOWORD(wParam) == IDC_WDREMOVE) {

        RemoveWaypoint(SelectedWaypoint);

        ::ReleaseDC(hDlg, hdcScreen);
        EndDialog(hDlg, LOWORD(wParam));
        break;
      }
      if (LOWORD(wParam) == IDC_WDINSERT) {

        InsertWaypoint(SelectedWaypoint);

        ::ReleaseDC(hDlg, hdcScreen);
        EndDialog(hDlg, LOWORD(wParam));
        break;
      }
      if (LOWORD(wParam) == IDC_WDSETHOME) {

	HomeWaypoint = SelectedWaypoint;
	SetToRegistry(szRegistryHomeWaypoint,HomeWaypoint);

        ::ReleaseDC(hDlg, hdcScreen);
        EndDialog(hDlg, LOWORD(wParam));
        break;
      }
      if (LOWORD(wParam) == IDC_WAYPOINTDETAILSNEXT) {
        page++;
        if (page==4) {
          page=0;
        }
      }
    case WM_PAINT:

      // make background white
      GetClientRect(hDlg, &rc);
      hdc = BeginPaint(hDlg, &ps);

      HGDIOBJ gTemp;
      gTemp = SelectObject(hdcScreen, GetStockObject(WHITE_BRUSH));
      SelectObject(hdcScreen, GetStockObject(WHITE_PEN));
      Rectangle(hdcScreen,rc.left,rc.top,rc.right,rc.bottom);
      EndPaint(hDlg, &ps);

      if (page==3) {
        ShowWindow(GetDlgItem(hDlg,IDC_WDGOTO),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREPLACE),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREMOVE),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDSETHOME),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDINSERT),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDTEXT),SW_HIDE);

        GetClientRect(hDlg, &rc);
        hdc = BeginPaint(hDlg, &ps);
        jpgimage1.Draw (hdcScreen, 0, 45,
			rc.right - rc.left, rc.bottom -  rc.top-45);
        EndPaint(hDlg, &ps);
      }
      if (page==2) {
        ShowWindow(GetDlgItem(hDlg,IDC_WDGOTO),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREPLACE),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREMOVE),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDSETHOME),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDINSERT),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDTEXT),SW_HIDE);

        GetClientRect(hDlg, &rc);
        hdc = BeginPaint(hDlg, &ps);
        jpgimage2.Draw (hdcScreen, 0, 45, -1, -1);
        EndPaint(hDlg, &ps);
      }
      if (page==0) {
        ShowWindow(GetDlgItem(hDlg,IDC_WDGOTO),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREPLACE),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREMOVE),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDSETHOME),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDINSERT),SW_HIDE);
        ShowWindow(GetDlgItem(hDlg,IDC_WDTEXT),SW_SHOW);
      }
      if (page==1) {

        ShowWindow(GetDlgItem(hDlg,IDC_WDGOTO),SW_SHOW);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREMOVE),SW_SHOW);
        ShowWindow(GetDlgItem(hDlg,IDC_WDREPLACE),SW_SHOW);
        ShowWindow(GetDlgItem(hDlg,IDC_WDSETHOME),SW_SHOW);
        ShowWindow(GetDlgItem(hDlg,IDC_WDINSERT),SW_SHOW);
        ShowWindow(GetDlgItem(hDlg,IDC_WDTEXT),SW_HIDE);
      }

      return FALSE;

    case WM_CLOSE:
      MapDirty = true;
      FullScreen();

    }
  return FALSE;
}




LRESULT CALLBACK LoggerDetails(HWND hDlg, UINT message,
			       WPARAM wParam, LPARAM lParam)
{
  LPWINDOWPOS lpwp;
  TCHAR Temp[100];

  switch (message)
    {
    case WM_INITDIALOG:

      GetRegistryString(szRegistryPilotName, Temp, 100);
      SetDlgItemText(hDlg,IDC_PILOTNAME,Temp);
      GetRegistryString(szRegistryAircraftType, Temp, 100);
      SetDlgItemText(hDlg,IDC_AIRCRAFTTYPE,Temp);
      GetRegistryString(szRegistryAircraftRego, Temp, 100);
      SetDlgItemText(hDlg,IDC_AIRCRAFTREGO,Temp);
      return TRUE;

    case WM_WINDOWPOSCHANGED:
      lpwp = (LPWINDOWPOS)(lParam);
      if(( lpwp->flags & SWP_HIDEWINDOW) == SWP_HIDEWINDOW)
        {
          GetDlgItemText(hDlg,IDC_PILOTNAME,Temp,100);
          SetRegistryString(szRegistryPilotName,Temp);
          GetDlgItemText(hDlg,IDC_AIRCRAFTTYPE,Temp,100);
          SetRegistryString(szRegistryAircraftType,Temp);
          GetDlgItemText(hDlg,IDC_AIRCRAFTREGO,Temp,100);
          SetRegistryString(szRegistryAircraftRego,Temp);
        }
      break;
    }
  return FALSE;
}



// ARH: Status Message functions
// Used to show a brief status message to the user
// Could be used to display debug messages
// or info messages like "Map panning OFF"
/////////////////////////////////////////////////////
WNDPROC fnOldStatusMsgWndProc;

// Intercept messages destined for the Status Message window
LRESULT CALLBACK StatusMsgWndTimerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

  switch (message) {
  case WM_TIMER :         // Fall through
  case WM_LBUTTONDOWN :
    DestroyWindow(hwnd);
    break;
  }

  // Pass message on to original window proc
  return CallWindowProc(fnOldStatusMsgWndProc, hwnd, message, wParam, lParam);
}




// Pop up a text dialog for a specified time
// period in milliseconds
//
// If delay_ms==0 the window will stay up until clicked
//
// Font size is (optionally) set using iFontHeightRatio
// - specifies the number of rows of text that would fit
// on the entire screen
//
// Insert linebreaks by using carriage return AND
// linefeed characters.  ie TEXT("Line 1\r\nLine 2")
// otherwise you'll get funny characters appearing
void ShowStatusMessage(TCHAR* text, int delay_ms, int iFontHeightRatio) {

  HWND hWnd;
  HFONT hFont;
  LOGFONT logfont;
  RECT rc;

  int fontHeight;
  int widthMain, heightMain;
  int widthStatus, heightStatus;
  int linecount;

  // Check inputs are valid
  if (delay_ms < 0) return;

  if (iFontHeightRatio < 2)  iFontHeightRatio = 2;
  if (iFontHeightRatio > 20) iFontHeightRatio = 20;

  // Get size of main window
  GetClientRect(hWndMainWindow, &rc);
  widthMain  = rc.right - rc.left;
  heightMain = rc.bottom - rc.top;


  // Build a font of the correct height
  fontHeight = (int)((rc.bottom-rc.top)/iFontHeightRatio);

  memset ((char *)&logfont, 0, sizeof (logfont));
  logfont.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE  ;
  logfont.lfHeight = fontHeight;
  logfont.lfWidth =  0;
  logfont.lfWeight = FW_BOLD;

#ifndef NOCLEARTYPE
  logfont.lfQuality = CLEARTYPE_COMPAT_QUALITY;
#endif

  hFont = CreateFontIndirect (&logfont);


  // Create a child window to contain status message
  hWnd = CreateWindow(TEXT("EDIT"), text,
    WS_VISIBLE|WS_CHILD|ES_MULTILINE|ES_CENTER|WS_BORDER|ES_READONLY,
    0,0,0,0,hWndMainWindow,NULL,hInst,NULL);

  // Apply font to window
  SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));

  // Now find out what size the window needs to be
  widthStatus  = (int)((double)widthMain * 0.95);
  heightStatus = (int)((double)fontHeight * 1.2);

  // Center it in the middle of the Main Window
  SetWindowPos(hWnd,HWND_TOPMOST,
	  (widthMain-widthStatus)/2, (heightMain-heightStatus)/2,
    widthStatus, heightStatus,
    SWP_SHOWWINDOW);

  // If there are multiple lines of text when using the current
  // width, then we need to increase the height and reposition
  linecount = SendMessage(hWnd, EM_GETLINECOUNT, 0, 0);

  if (linecount > 1) {
    heightStatus = heightStatus * linecount;

    if (heightStatus > heightMain) heightStatus = heightMain;

    SetWindowPos(hWnd,HWND_TOPMOST,
  	  (widthMain-widthStatus)/2, (heightMain-heightStatus)/2,
      widthStatus, heightStatus,
      SWP_SHOWWINDOW);
  }



  // Subclass window function so that we can trap timer messages
  fnOldStatusMsgWndProc = (WNDPROC) SetWindowLong(hWnd, GWL_WNDPROC, (LONG) StatusMsgWndTimerProc) ;

  if (delay_ms) {
    // Set timer to specified timeout.
    // Window will close when timer fires
    if (!SetTimer(hWnd, 1, delay_ms, NULL))
      DestroyWindow(hWnd);  // Couldn't init timer
  }

  // FINALLY, display the window for the user's perusal
  ShowWindow(hWnd,SW_SHOW);
  UpdateWindow(hWnd);

}




bool startupfinished = false;

// Intercept messages destined for the Status Message window
LRESULT CALLBACK StartupWndTimerProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

  switch (message) {
    case WM_COMMAND:
      switch (LOWORD(wParam))
        {
        case IDC_SPLASH:
          DestroyWindow(hwnd);
          return TRUE;
        };
      break;
  case WM_TIMER :         // Fall through to destroy
  case WM_LBUTTONDOWN :
    DestroyWindow(hwnd);
    break;
  case WM_CLOSE:
    startupfinished = true;
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}


// Pop up start up screen
//


void OpenStartupScreen() {

  HWND hWnd;

  startupfinished = false;

  // Create a child window to contain status message
  hWnd =
    CreateDialog(hInst, (LPCTSTR)IDD_SPLASH, hWndMainWindow,
                 (DLGPROC)StartupWndTimerProc);

  SetWindowPos(hWndMainWindow,HWND_TOPMOST,0,0,
               GetSystemMetrics(SM_CXSCREEN),
               GetSystemMetrics(SM_CYSCREEN),
               SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);

  // Subclass window function so that we can trap timer messages
  // set timeout to 3 seconds
  SetTimer(hWnd, 2, 3000, NULL);

  MSG msg;

  while (GetMessage(&msg, NULL, 0, 0) && !startupfinished)
    {
      DispatchMessage(&msg);
    }

}



void StartupScreen() {
  DWORD dwThreadID;
  HANDLE splashthread;
  splashthread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE )
                               OpenStartupScreen, 0, 0, &dwThreadID);

  CloseHandle (splashthread);

}

