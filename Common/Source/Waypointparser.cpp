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
#include "Waypointparser.h"
#include "externs.h"
#include "Dialogs.h"
#include "resource.h"
#include "Utils.h"


#include <windows.h>
#include <Commctrl.h>

#include <tchar.h>

static void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber);
static int ParseWayPointString(TCHAR *TempString,WAYPOINT *Temp);
static double CalculateAngle(TCHAR *temp);
static int CheckFlags(TCHAR *temp);
static double ReadAltitude(TCHAR *temp);

static TCHAR TempString[210];

void ReadWayPointFile(HANDLE hFile)
{
  int WayPointCount = 0;
  WAYPOINT Temp;
  WAYPOINT *List;
  TCHAR szTemp[100];

  int Tick = 0; int Tock=0;
  DWORD fSize, fPos=0;

  HWND hProgress;

  hProgress=CreateDialog(hInst,(LPCTSTR)IDD_PROGRESS,hWndMainWindow,(DLGPROC)Progress);
  SetDlgItemText(hProgress,IDC_MESSAGE,TEXT("Loading Waypoints File..."));

  SetWindowPos(hProgress,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
  ShowWindow(hProgress,SW_SHOW);
  UpdateWindow(hProgress);


  fSize = GetFileSize(hFile,NULL);
  if (!fSize) {
    return;
  }

  Temp.Details = NULL;

  while(ReadString(hFile,200,TempString))
    {
      Tock++; Tock %= 300;
      if(Tock == 0)
	{
	  fPos = SetFilePointer(hFile,0,NULL,FILE_CURRENT);
	  fPos *= 100;	fPos /= 2*fSize;

	  wsprintf(szTemp,TEXT("%d%%"),int(fPos));
	  SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);

	  Tick ++; Tick %=100;
	}

      if(_tcsstr(TempString,TEXT("**")) != TempString) // Look For Comment
	{
	  if(ParseWayPointString(TempString,&Temp))
	    {
	      WayPointCount ++;
	    }
	}
    }

  if(WayPointCount)
    {
      WayPointList = (WAYPOINT *)LocalAlloc(LPTR, WayPointCount * sizeof(WAYPOINT));

      if(WayPointList != NULL)
	{
	  List = WayPointList;
	  SetFilePointer(hFile,0,NULL,FILE_BEGIN);

	  while(ReadString(hFile,200,TempString))
	    {
	      Tock++; Tock %= 300;
	      if(Tock == 0)
		{
		  fPos = SetFilePointer(hFile,0,NULL,FILE_CURRENT);
		  fPos *= 100;	fPos /= 2*fSize;fPos += 50;
		  wsprintf(szTemp,TEXT("%d%%"),int(fPos));
		  SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);
		  Tick ++; Tick %=100;
		}
	      if(_tcsstr(TempString,TEXT("**")) != TempString) // Look For Comment
		{
                  List->Details = NULL;
		  if (ParseWayPointString(TempString,List)) {
		    List ++;
		    // bug fix by Samuel Gisiger
		  }
		}
	    }
	  NumberOfWayPoints = WayPointCount;
	  DestroyWindow(hProgress);
	}
      else
	{
	  DestroyWindow(hProgress);
	  MessageBox(hWndMainWindow,TEXT("Not Enough Memory For Waypoints"),TEXT("Error"),MB_OK|MB_ICONSTOP);
	}
    }
  else
    {
      DestroyWindow(hProgress);
      //		MessageBox(hWndMainWindow,TEXT("No Waypoints Found"),TEXT("Warning"),MB_OK|MB_ICONINFORMATION);
    }
}


int ParseWayPointString(TCHAR *TempString,WAYPOINT *Temp)
{
  TCHAR ctemp[80];
  TCHAR *Zoom;


  ExtractParameter(TempString,ctemp,0);
  Temp->Number = _tcstol(ctemp, &Zoom, 10);


  ExtractParameter(TempString,ctemp,1); //Lattitude
  Temp->Lattitude = CalculateAngle(ctemp);
  if((Temp->Lattitude > 90) || (Temp->Lattitude < -90))
    {
      return FALSE;
    }


  ExtractParameter(TempString,ctemp,2); //Longditude
  Temp->Longditude  = CalculateAngle(ctemp);
  if((Temp->Longditude  > 180) || (Temp->Longditude  < -180))
    {
      return FALSE;
    }

  ExtractParameter(TempString,ctemp,3); //Altitude
  Temp->Altitude = ReadAltitude(ctemp);

  ExtractParameter(TempString,ctemp,4); //Flags
  Temp->Flags = CheckFlags(ctemp);

  ExtractParameter(TempString,ctemp,5); // Name
  _tcscpy(Temp->Name, ctemp);

  ExtractParameter(TempString,ctemp,6); // Comment
  ctemp[COMMENT_SIZE] = '\0';

  Temp->Zoom = 0;
  Zoom = _tcschr(ctemp,'*');
  if(Zoom)
    {
      *Zoom = '\0';
      Zoom +=2;
      Temp->Zoom = _tcstol(Zoom, &Zoom, 10);
    }

  // sgi, move "panic-stripping" of the comment-field after we extract
  // the zoom factor
  ctemp[COMMENT_SIZE] = '\0';
  _tcscpy(Temp->Comment, ctemp);

  LockTerrainDataGraphics();
  terrain_dem_graphics.SetTerrainRounding(0.0);
  double myalt =
    terrain_dem_graphics.GetTerrainHeight(Temp->Lattitude , Temp->Longditude);
  UnlockTerrainDataGraphics();

  if(Temp->Altitude == 0)
    Temp->Altitude = myalt;

  if (Temp->Details) {
    free(Temp->Details);
  }

  return TRUE;
}


void ExtractParameter(TCHAR *Source, TCHAR *Destination, int DesiredFieldNumber)
{
  int index = 0;
  int dest_index = 0;
  int CurrentFieldNumber = 0;
  int StringLength        = 0;

  StringLength = _tcslen(Source);

  while( (CurrentFieldNumber < DesiredFieldNumber) && (index < StringLength) )
    {
      if ( Source[ index ] == ',' )
	{
	  CurrentFieldNumber++;
	}
      index++;
    }

  if ( CurrentFieldNumber == DesiredFieldNumber )
    {
      while( (index < StringLength)    &&
	     (Source[ index ] != ',') &&
	     (Source[ index ] != 0x00) )
	{
	  Destination[dest_index] = Source[ index ];
	  index++; dest_index++;
	}
      Destination[dest_index] = '\0';
    }
  // strip trailing spaces
  for (int i=dest_index-1; i>0; i--) {
    if (Destination[i]==' ') {
      Destination[i]= '\0';
    } else return;
  }
}

static double CalculateAngle(TCHAR *temp)
{
  TCHAR *Colon;
  TCHAR *Stop;
  double Degrees, Mins;

  Colon = _tcschr(temp,':');

  if(!Colon)
    {
      return -9999;
    }

  *Colon =  NULL;
  Colon ++;

  Degrees = (double)_tcstol(temp, &Stop, 10);
  Mins = (double)StrToDouble(Colon, &Stop);
  if (*Stop == ':') {
    Mins += ((double)_tcstol(++Stop, &Stop, 10)/60.0);
  }

  Degrees += (Mins/60);

  if((*Stop == 'N') || (*Stop == 'E'))
    {
    }
  else if((*Stop == 'S') || (*Stop == 'W'))
    {
      Degrees *= -1;
    }
  else
    {
      return -9999;
    }

  return Degrees;
}

static int CheckFlags(TCHAR *temp)
{
  int Flags = 0;

  if(_tcschr(temp,'A')) Flags += AIRPORT;
  if(_tcschr(temp,'T')) Flags += TURNPOINT;
  if(_tcschr(temp,'L')) Flags += LANDPOINT;
  if(_tcschr(temp,'H')) Flags += HOME;
  if(_tcschr(temp,'S')) Flags += START;
  if(_tcschr(temp,'F')) Flags += FINISH;
  if(_tcschr(temp,'R')) Flags += RESTRICTED;
  if(_tcschr(temp,'W')) Flags += WAYPOINTFLAG;

  return Flags;
}


static double ReadAltitude(TCHAR *temp)
{
  TCHAR *Stop;
  double Altitude;


  Altitude = (double)_tcstol(temp, &Stop, 10);

  if(*Stop == 'F')
    {
      Altitude = Altitude / TOFEET;
    }
  return Altitude;
}

extern TCHAR szRegistryWayPointFile[];
void ReadWayPoints(void)
{
  TCHAR	szFile[MAX_PATH] = TEXT("\0");

  HANDLE hFile;

  LockFlightData();

  GetRegistryString(szRegistryWayPointFile, szFile, MAX_PATH);

  hFile = CreateFile(szFile,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);


  if(hFile != INVALID_HANDLE_VALUE )
    {
      ReadWayPointFile(hFile);
      CloseHandle (hFile);
    }

  UnlockFlightData();

}


void SetHome(void)
{
  TCHAR szRegistryHomeWaypoint[]= TEXT("HomeWaypoint");
  unsigned int i;

  // check invalid home waypoint
  if((HomeWaypoint <0)||(HomeWaypoint >= (int)NumberOfWayPoints))
    {
      HomeWaypoint = -1;
    }

  // search for home in waypoint list
  for(i=0;i<NumberOfWayPoints;i++)
    {
      if( (WayPointList[i].Flags & HOME) == HOME)
	  {
	    if (HomeWaypoint== -1) {
	      HomeWaypoint = i;
	    }
	  }
    }

//
// If we haven't found the waypoint or the homewaypoint is out of
// range then set it to the first in the waypoint list
//
  if (HomeWaypoint<0) {
    HomeWaypoint = 0;
  }

  // Assume here we have a home now...
  GPS_INFO.Lattitude = WayPointList[HomeWaypoint].Lattitude;
  GPS_INFO.Longditude = WayPointList[HomeWaypoint].Longditude;


  //
  // Save the home waypoint number in the resgistry
  //
  SetToRegistry(szRegistryHomeWaypoint,HomeWaypoint);
}


int FindNearestWayPoint(double X, double Y, double MaxRange)
{
  unsigned int i;
  int NearestIndex = 0;
  double	NearestDistance, Dist;

  if(NumberOfWayPoints ==0)
    {
      return -1;
    }

  NearestDistance = Distance(Y,X,WayPointList[0].Lattitude, WayPointList[0].Longditude);
  for(i=1;i<NumberOfWayPoints;i++)
    {
      if (((WayPointList[i].Zoom >= MapScale*10)||
	   (WayPointList[i].Zoom == 0))
	   &&(MapScale <= 10)) {

	    // only look for visible waypoints
	    // feature added by Samuel Gisiger
	    Dist = Distance(Y,X,WayPointList[i].Lattitude, WayPointList[i].Longditude);
	    if(Dist < NearestDistance)
	      {
		NearestIndex = i;
		NearestDistance = Dist;
	      }
	  }
    }
  if(NearestDistance < MaxRange)
    return NearestIndex;
  else
    return -1;
}





