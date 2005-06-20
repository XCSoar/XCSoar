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
#include "Airspace.h"
#include "externs.h"
#include "dialogs.h"
#include "Utils.h"
#include "XCSoar.h"

#include <windows.h>
#include <Commctrl.h>
#include <math.h>

#include <tchar.h>

#include "resource.h"

static int StartsWith(TCHAR *Text, TCHAR *LookFor);
static void ReadCoords(TCHAR *Text, double *X, double *Y);
static void AddAirspaceCircle(AIRSPACE_AREA *Temp, double CenterX, double CenterY, double Radius);
static void AddPoint(AIRSPACE_POINT *Temp);
static void AddArea(AIRSPACE_AREA *Temp);
static void ReadAltitude(TCHAR *Text, AIRSPACE_ALT *Alt);
static void CalculateArc(TCHAR *Text);
static void CalculateSector(TCHAR *Text);
static void ParseError(TCHAR *Line);


static int GetNextLine(HANDLE hFile, TCHAR *Text);
static int Waiting(TCHAR *Text, AIRSPACE_AREA *Temp);
static int Command_AC(TCHAR *Text, AIRSPACE_AREA *Temp);
static int Command_AN(TCHAR *Text, AIRSPACE_AREA *Temp);
static int Command_AH(TCHAR *Text, AIRSPACE_AREA *Temp);
static int Command_AL(TCHAR *Text, AIRSPACE_AREA *Temp);
static int Command_V (TCHAR *Text, AIRSPACE_AREA *Temp);
static int Command_DP(TCHAR *Text, AIRSPACE_AREA *Temp);


#define WAITING 0
#define COMMAND_AC 1
#define COMMAND_AN 2
#define COMMAND_AL 3
#define COMMAND_AH 4
#define COMMAND_V  5
#define COMMAND_DC 6
#define COMMAND_DP 7 

#define COUNT 0
#define FILL 1
static int ReadMode = COUNT;

static TCHAR TempString[200];

static AIRSPACE_AREA TempArea;
static AIRSPACE_POINT TempPoint;
static int Rotation = 1;
static double CenterX = 0;
static double CenterY = 0;
static float Radius = 0;
static float Width = 0;
static float Zoom = 0;


void ReadAirspace(HANDLE hFile)
{
	int Mode = WAITING;
	int Tick = 0; int Tock=0;
	HWND hProgress;
	double fSize, fPos;
	DWORD dwPos;
	TCHAR szTemp[100];

	ReadMode = COUNT;
	hProgress=CreateDialog(hInst,(LPCTSTR)IDD_PROGRESS,hWndMainWindow,(DLGPROC)Progress);
	SetDlgItemText(hProgress,IDC_MESSAGE,TEXT("Loading Airspace File"));
	SetWindowPos(hProgress,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
	ShowWindow(hProgress,SW_SHOW);
	UpdateWindow(hProgress);

	fSize = (double)GetFileSize(hFile,NULL);

	TempArea.FirstPoint = 0;
	while(GetNextLine(hFile,TempString))
	{
		Tock++; Tock %= 400;
		if(Tock == 0)
		{
			dwPos = SetFilePointer(hFile,0,NULL,FILE_CURRENT);
			//dwPos = LOWORD(dwPos);
			fPos = dwPos * 100;
			fPos = fPos / (2*fSize);
			wsprintf(szTemp,TEXT("%d%%"),int(fPos));	
			SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);
			Tick ++; Tick %=100;
		}

		switch (Mode)
		{
			case WAITING :		Mode = Waiting(TempString, &TempArea); break;
			case COMMAND_AC:	Mode = Command_AC(TempString, &TempArea); break;
			case COMMAND_AN:	Mode = Command_AN(TempString, &TempArea); break;
			case COMMAND_AL:	Mode = Command_AL(TempString, &TempArea); break;
			case COMMAND_AH:	Mode = Command_AH(TempString, &TempArea); break;
			case COMMAND_V :	Mode = Command_V (TempString, &TempArea); break;
			case COMMAND_DP:	Mode = Command_DP(TempString, &TempArea); break;

			default : Mode = WAITING;
		}
	}

	AirspaceCircle = (AIRSPACE_CIRCLE *)LocalAlloc(LMEM_FIXED, NumberOfAirspaceCircles * sizeof(AIRSPACE_CIRCLE));
	AirspacePoint = (AIRSPACE_POINT *) LocalAlloc(LMEM_FIXED, NumberOfAirspacePoints  * sizeof(AIRSPACE_POINT));
	AirspaceArea  = (AIRSPACE_AREA *)  LocalAlloc(LMEM_FIXED, NumberOfAirspaceAreas   * sizeof(AIRSPACE_AREA));

	if(( AirspaceCircle == NULL) || (AirspacePoint == NULL) || (AirspaceArea == NULL))
	{
		NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
		if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
		if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
		if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);
		DestroyWindow(hProgress);
		return;
	}
	
	Mode = WAITING;
	ReadMode = FILL;
	TempArea.FirstPoint = 0;
	NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
	SetFilePointer(hFile,0,NULL,FILE_BEGIN);
	
	while(GetNextLine(hFile,TempString))
	{
		Tock++; Tock %= 400;
		if(Tock == 0)
		{
			fPos = SetFilePointer(hFile,0,NULL,FILE_CURRENT);
			//fPos = LOWORD(fPos);
			fPos *= 100;
			fPos = fPos / (2*fSize);
			fPos += 50;
			wsprintf(szTemp,TEXT("%d%%"),int(fPos));	
			SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);
			Tick ++; Tick %=100;
		}

		switch (Mode)
		{
			case WAITING :		Mode = Waiting(TempString, &TempArea); break;
			case COMMAND_AC:	Mode = Command_AC(TempString, &TempArea); break;
			case COMMAND_AN:	Mode = Command_AN(TempString, &TempArea); break;
			case COMMAND_AL:	Mode = Command_AL(TempString, &TempArea); break;
			case COMMAND_AH:	Mode = Command_AH(TempString, &TempArea); break;
			case COMMAND_V :	Mode = Command_V (TempString, &TempArea); break;
			case COMMAND_DP:	Mode = Command_DP(TempString, &TempArea); break;

			default : Mode = WAITING;
		}
	}
	DestroyWindow(hProgress);
}


int GetNextLine(HANDLE hFile, TCHAR *Text)
{
	TCHAR *Comment;
	int size;

	while(ReadString(hFile,200,TempString))
	{
		if(_tcslen(TempString) <=2)
		{
		}
		else if(StartsWith(TempString,TEXT("AT ")))
		{
		}
		else if(StartsWith(TempString,TEXT("SP ")))
		{
		}
		else if(StartsWith(TempString,TEXT("SB ")))
		{
		}
		else if(StartsWith(TempString,TEXT("V Z=")))
		{
		}
		else if(StartsWith(TempString,TEXT("TC ")))
		{
		}
		else if(StartsWith(TempString,TEXT("TO ")))
		{
		}
		else if(!StartsWith(TempString,TEXT("V T=1")))
		{
			if(Text[0] != '*')
			{
				Comment = _tcschr(Text,'*');
				if(Comment)
				{
					*Comment = '\0';
				}
				size = _tcsclen(Text); 
				if(Text[size-1] == '\n') 
					Text[--size] = '\0';
				if(Text[size-1] == '\r') 
					Text[--size] = '\0';
				return TRUE;
			}
		}
	}
	return FALSE;
}

static int Waiting(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	TempArea.NumPoints = 0;
	if(StartsWith(Text,TEXT("AC ")))
	{
		     if(StartsWith(&Text[3],TEXT("R"))) Temp->Type = RESTRICT;
		else if(StartsWith(&Text[3],TEXT("Q"))) Temp->Type = DANGER;
		else if(StartsWith(&Text[3],TEXT("P"))) Temp->Type = PROHIBITED;
		else if(StartsWith(&Text[3],TEXT("A"))) Temp->Type = CLASSA;
		else if(StartsWith(&Text[3],TEXT("B"))) Temp->Type = CLASSB;
		else if(StartsWith(&Text[3],TEXT("CTR"))) Temp->Type = CTR;
		else if(StartsWith(&Text[3],TEXT("D"))) Temp->Type = CLASSD;
		else if(StartsWith(&Text[3],TEXT("GP"))) Temp->Type = NOGLIDER;
		else if(StartsWith(&Text[3],TEXT("W"))) Temp->Type = WAVE;
		else Temp->Type = OTHER;

		Rotation = +1;
		return COMMAND_AC;
	}
	return WAITING;
}

static int Command_AC(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	if(StartsWith(Text,TEXT("AN ")))
	{
		Text[NAME_SIZE] = '\0';
		_tcscpy(Temp->Name, &Text[3]);
		return COMMAND_AN;
	}
	ParseError(Text);
	return WAITING;
}

static int Command_AN(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	if(StartsWith(Text,TEXT("AL ")))
	{
		ReadAltitude(&Text[3], &Temp->Base);
		return COMMAND_AL;
	}
	
	if(StartsWith(Text,TEXT("AH ")))
	{
		ReadAltitude(&Text[3],&Temp->Top);
		return COMMAND_AH;
	}
	ParseError(Text);
	return WAITING;
}

static int Command_AL(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	if(StartsWith(Text,TEXT("AH ")))
	{
		ReadAltitude(&Text[3],&Temp->Top);
		return COMMAND_AH;
	}
	
	if(StartsWith(Text,TEXT("V ")))
	{
		if(StartsWith(Text,TEXT("V X=")))
		{
			ReadCoords(&Text[4],&CenterX, &CenterY);
			return COMMAND_V;
		}
		if(StartsWith(Text,TEXT("V D=-")))
		{
			Rotation = -1;
			return COMMAND_V;
		}
		if(StartsWith(Text,TEXT("V D=+")))
		{
			Rotation = +1;
			return COMMAND_V;
		}
	}
	
	if(StartsWith(Text,TEXT("DP ")))
	{
		ReadCoords(&Text[3],&TempPoint.Longditude , &TempPoint.Lattitude );
		AddPoint(&TempPoint);
		TempArea.NumPoints++;
		return COMMAND_DP;
	}
	
	if(StartsWith(Text,TEXT("DB ")))
	{
		CalculateArc(Text);
		return COMMAND_DP;
	}

	if(StartsWith(Text,TEXT("DA ")))
	{
		CalculateSector(Text);
		return COMMAND_DP;
	}

	ParseError(Text);
	return WAITING;
}

static int Command_AH(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	if(StartsWith(Text,TEXT("AL ")))
	{
		ReadAltitude(&Text[3],&Temp->Base);
		return COMMAND_AL;
	}

	if(StartsWith(Text,TEXT("V ")))
	{
		if(StartsWith(Text,TEXT("V X=")))
		{
			ReadCoords(&Text[4],&CenterX, &CenterY);
			return COMMAND_V;
		}
		if(StartsWith(Text,TEXT("V D=-")))
		{
			Rotation = -1;
			return COMMAND_V;
		}
		if(StartsWith(Text,TEXT("V D=+")))
		{
			Rotation = +1;
			return COMMAND_V;
		}
	}
	
	if(StartsWith(Text,TEXT("DP ")))
	{
		ReadCoords(&Text[3],&TempPoint.Longditude , &TempPoint.Lattitude );
		AddPoint(&TempPoint);
		TempArea.NumPoints++;
		return COMMAND_DP;
	}
	if(StartsWith(Text,TEXT("DB ")))
	{
		CalculateArc(Text);
		return COMMAND_DP;
	}

	if(StartsWith(Text,TEXT("DA ")))
	{
		CalculateSector(Text);
		return COMMAND_DP;
	}


	ParseError(Text);
	return WAITING;
}

static int Command_V(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	if(StartsWith(Text,TEXT("DC ")))
	{
		Radius = (float)StrToDouble(&Text[2],NULL);
		Radius = (float)(Radius * NAUTICALMILESTOMETRES);
		AddAirspaceCircle(Temp, CenterX, CenterY, Radius);
		return WAITING;
	}
	
	if(StartsWith(Text,TEXT("V D=-")))
	{
		Rotation = -1;
		return COMMAND_V;
	}
	
	if(StartsWith(Text,TEXT("V D=+")))
	{
		Rotation = +1;
		return COMMAND_V;
	}
	if(StartsWith(Text,TEXT("V X=")))
	{
		ReadCoords(&Text[4],&CenterX, &CenterY);
		return COMMAND_V;
	}
	if(StartsWith(Text,TEXT("DB ")))
	{
		CalculateArc(Text);
		return COMMAND_DP;
	}

	if(StartsWith(Text,TEXT("DA ")))
	{
		CalculateSector(Text);
		return COMMAND_DP;
	}

	ParseError(Text);
	return WAITING;
}
	

static int Command_DP(TCHAR *Text, AIRSPACE_AREA *Temp)
{
	if(StartsWith(Text,TEXT("DP ")))
	{
		ReadCoords(&Text[3],&TempPoint.Longditude , &TempPoint.Lattitude );
		AddPoint(&TempPoint);
		TempArea.NumPoints++;
		return COMMAND_DP;
	}
	if(StartsWith(Text,TEXT("AC ")))
	{
		AddArea(&TempArea);
		TempArea.NumPoints = 0;
		if(StartsWith(&Text[3],TEXT("R"))) Temp->Type = RESTRICT;
		else if(StartsWith(&Text[3],TEXT("Q"))) Temp->Type = DANGER;
		else if(StartsWith(&Text[3],TEXT("P"))) Temp->Type = PROHIBITED;
		else if(StartsWith(&Text[3],TEXT("A"))) Temp->Type = CLASSA;
		else if(StartsWith(&Text[3],TEXT("B"))) Temp->Type = CLASSB;
		else if(StartsWith(&Text[3],TEXT("CTR"))) Temp->Type = CTR;
		else if(StartsWith(&Text[3],TEXT("C"))) Temp->Type = CLASSC;
		else if(StartsWith(&Text[3],TEXT("D"))) Temp->Type = CLASSD;
		else if(StartsWith(&Text[3],TEXT("GP"))) Temp->Type = NOGLIDER;
		else if(StartsWith(&Text[3],TEXT("W"))) Temp->Type = WAVE;
		else Temp->Type = OTHER;

		Rotation = 1;

		return COMMAND_AC;
	}
	
	if(StartsWith(Text,TEXT("V X=")))
	{
		ReadCoords(&Text[4],&CenterX, &CenterY);
		return COMMAND_DP;
	}
	
	if(StartsWith(Text,TEXT("V D=-")))
	{
		Rotation = -1;
		return COMMAND_V;
	}
	
	if(StartsWith(Text,TEXT("V D=+")))
	{
		Rotation = +1;
		return COMMAND_V;
	}
	
	if(StartsWith(Text,TEXT("DB ")))
	{
		CalculateArc(Text);
		return COMMAND_DP;
	}

	if(StartsWith(Text,TEXT("DA ")))
	{
		CalculateSector(Text);
		return COMMAND_DP;
	}

	ParseError(Text);
	return WAITING;
}
	

static int StartsWith(TCHAR *Text, TCHAR *LookFor)
{
	if(_tcsstr(Text,LookFor) == Text)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


static void ReadCoords(TCHAR *Text, double *X, double *Y)
{
	double Ydeg=0, Ymin=0, Ysec=0;
	double Xdeg=0, Xmin=0, Xsec=0;
	TCHAR *Stop;
	
	Ydeg = (double)StrToDouble(Text, &Stop);
	Stop++;
	Ymin = (double)StrToDouble(Stop, &Stop);
	if(*Stop == ':')
	{
		Stop++;
		Ysec = (double)StrToDouble(Stop, &Stop);
	}

	*Y = Ysec/3600 + Ymin/60 + Ydeg;

	Stop ++;
	if((*Stop == 'S') || (*Stop == 's'))
	{
		*Y = *Y * -1;
	}
	Stop++;

	Xdeg = (double)StrToDouble(Stop, &Stop);
	Stop++;
	Xmin = (double)StrToDouble(Stop, &Stop);
	if(*Stop == ':')
	{
		Stop++;
		Xsec = (double)StrToDouble(Stop, &Stop);
	}

	*X = Xsec/3600 + Xmin/60 + Xdeg;

	Stop ++;
	if((*Stop == 'W') || (*Stop == 'w'))
	{
		*X = *X * -1;
	}
}

static void AddAirspaceCircle(AIRSPACE_AREA *Temp, double CenterX, double CenterY, double Radius)
{
	AIRSPACE_CIRCLE *NewCircle = NULL;

	if(ReadMode == COUNT)
	{
		NumberOfAirspaceCircles++;
	}
	else
	{
		NewCircle =  &AirspaceCircle[NumberOfAirspaceCircles];
		NumberOfAirspaceCircles++;

		_tcscpy(NewCircle->Name , Temp->Name);
		NewCircle->Lattitude = CenterY;
		NewCircle->Longditude = CenterX;
		NewCircle->Radius = Radius;
		NewCircle->Type = Temp->Type;
		NewCircle->Top.Altitude  = Temp->Top.Altitude ;
		NewCircle->Top.FL   = Temp->Top.FL;
		NewCircle->Base.Altitude  = Temp->Base.Altitude;
		NewCircle->Base.FL   = Temp->Base.FL;
	}
}

static void AddPoint(AIRSPACE_POINT *Temp)
{
	AIRSPACE_POINT *NewPoint = NULL;

	if(ReadMode == COUNT)
	{
		NumberOfAirspacePoints++;
	}
	else
	{
		NewPoint = &AirspacePoint[NumberOfAirspacePoints];
		NumberOfAirspacePoints++;

		NewPoint->Lattitude  = Temp->Lattitude;
		NewPoint->Longditude = Temp->Longditude;
	}
}

static void AddArea(AIRSPACE_AREA *Temp)
{
	AIRSPACE_AREA *NewArea = NULL;
	AIRSPACE_POINT *PointList = NULL;
	unsigned i;
	

	if(ReadMode == COUNT)
	{
		NumberOfAirspaceAreas++;
	}
	else
	{
		NewArea = &AirspaceArea[NumberOfAirspaceAreas];
		NumberOfAirspaceAreas++;

		_tcscpy(NewArea->Name , Temp->Name);
		NewArea->Type = Temp->Type;
		NewArea->Base.Altitude  = Temp->Base.Altitude ;
		NewArea->Base.FL   = Temp->Base.FL  ;
		NewArea->NumPoints = Temp->NumPoints;
		NewArea->Top.Altitude  = Temp->Top.Altitude ;
		NewArea->Top.FL = Temp->Top.FL;
		NewArea->FirstPoint = Temp->FirstPoint;

		Temp->FirstPoint = Temp->FirstPoint + Temp->NumPoints ;

		PointList = &AirspacePoint[NewArea->FirstPoint];
		NewArea->MaxLattitude = -90;
		NewArea->MinLattitude = 90;
		NewArea->MaxLongditude  = -180;
		NewArea->MinLongditude  = 180;
		
		for(i=0;i<Temp->NumPoints; i++)
		{
			if(PointList[i].Lattitude > NewArea->MaxLattitude)
				NewArea->MaxLattitude = PointList[i].Lattitude ;
			if(PointList[i].Lattitude < NewArea->MinLattitude)
				NewArea->MinLattitude = PointList[i].Lattitude ;

			if(PointList[i].Longditude  > NewArea->MaxLongditude)
				NewArea->MaxLongditude  = PointList[i].Longditude ;
			if(PointList[i].Longditude  < NewArea->MinLongditude)
				NewArea->MinLongditude  = PointList[i].Longditude ;
		}
	}
}

static void ReadAltitude(TCHAR *Text, AIRSPACE_ALT *Alt)
{
	TCHAR *Stop;
	
		
	if(StartsWith(Text,TEXT("SFC")))
	{
		Alt->Altitude = 0;
		Alt->FL = 0;
	}
	else if(StartsWith(Text,TEXT("FL")))
	{
		Alt->FL = (double)StrToDouble(&Text[2], &Stop);
		Alt->Altitude = 100 * Alt->FL ;
		Alt->Altitude = Alt->Altitude/TOFEET;
	}
	else
	{
		Alt->Altitude = (double)StrToDouble(Text, &Stop);
		Alt->Altitude = Alt->Altitude/TOFEET;
		Alt->FL = 0;
	}
}

void CalculateSector(TCHAR *Text)
{
	double Radius;
	double StartBearing;
	double EndBearing;
	TCHAR *Stop;

	Radius = NAUTICALMILESTOMETRES * (double)StrToDouble(&Text[2], &Stop);
	StartBearing = (double)StrToDouble(&Stop[1], &Stop);
	EndBearing = (double)StrToDouble(&Stop[1], &Stop);

	while(fabs(EndBearing-StartBearing) > 7.5)
	{
		if(StartBearing >= 360)
			StartBearing -= 360;
		if(StartBearing < 0)
			StartBearing += 360;

		TempPoint.Lattitude =  FindLattitude(CenterY, CenterX, StartBearing, Radius);
		TempPoint.Longditude = FindLongditude(CenterY, CenterX, StartBearing, Radius);
		AddPoint(&TempPoint);
		TempArea.NumPoints++;

		StartBearing += Rotation *5 ;
	}
	TempPoint.Lattitude =  FindLattitude(CenterY, CenterX, EndBearing, Radius);
	TempPoint.Longditude = FindLongditude(CenterY, CenterX, EndBearing, Radius);
	AddPoint(&TempPoint);
	TempArea.NumPoints++;
}

void CalculateArc(TCHAR *Text)
{
	double StartLat, StartLon;
	double EndLat, EndLon;
	double StartBearing;
	double EndBearing;
	double Radius;
	TCHAR *Comma = NULL;

	ReadCoords(&Text[3],&StartLon , &StartLat);
	
	Comma = _tcschr(Text,',');
	if(!Comma)
		return;

	ReadCoords(&Comma[1],&EndLon , &EndLat);

	Radius = Distance(CenterY, CenterX, StartLat, StartLon);
	StartBearing = Bearing(CenterY, CenterX, StartLat, StartLon);
	EndBearing = Bearing(CenterY, CenterX, EndLat, EndLon);
	TempPoint.Lattitude  = StartLat;
	TempPoint.Longditude = StartLon;
	AddPoint(&TempPoint);
	TempArea.NumPoints++;

	while(fabs(EndBearing-StartBearing) > 7.5)
	{
		StartBearing += Rotation *5 ;

		if(StartBearing > 360)
			StartBearing -= 360;
		if(StartBearing < 0)
			StartBearing += 360;

		TempPoint.Lattitude =  FindLattitude(CenterY, CenterX, StartBearing, Radius);
		TempPoint.Longditude = FindLongditude(CenterY, CenterX, StartBearing, Radius);
		AddPoint(&TempPoint);
		TempArea.NumPoints++;
	}
	TempPoint.Lattitude  = EndLat;
	TempPoint.Longditude = EndLon;
	AddPoint(&TempPoint);
	TempArea.NumPoints++;
}
	

void ParseError(TCHAR *Line)
{
	TCHAR Message[200];
	
	wsprintf(Message,TEXT("Error in Secition %sat Line %s"),TempArea.Name,Line);

	MessageBox(hWndMainWindow,Message,TEXT("Error in Airspace File"),MB_OK|MB_ICONINFORMATION);
}

extern TCHAR szRegistryAirspaceFile[];
void ReadAirspace(void)
{
	TCHAR	szFile[MAX_PATH] = TEXT("\0");
	
	HANDLE hFile;

	GetRegistryString(szRegistryAirspaceFile, szFile, MAX_PATH);
	hFile = CreateFile(szFile,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
		
	if(hFile != INVALID_HANDLE_VALUE )
	
	{
		ReadAirspace(hFile);
		CloseHandle(hFile);
	}
}



int FindAirspaceCircle(double Longditude,double Lattitude)
{
	unsigned i;
	int NearestIndex = 0;
	double Dist;

	if(NumberOfAirspaceCircles == 0)
	{
		return -1;
	}
		
	for(i=0;i<NumberOfAirspaceCircles;i++)
	{
		if(AirspaceCircle[i].Visible)
		{
			Dist = Distance(Lattitude,Longditude,AirspaceCircle[i].Lattitude, AirspaceCircle[i].Longditude);
			if(Dist < AirspaceCircle[i].Radius )
			{
				if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Top.Altitude))
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int FindAirspaceArea(double Longditude,double Lattitude)
{
	unsigned i,j;
	double AngleSum;
	double LastAngle, NewAngle,Angle;
	unsigned XPoint, YPoint;
	unsigned LastPoint,FirstPoint;

	if(NumberOfAirspaceAreas == 0)
	{
		return -1;
	}

	for(i=0;i<NumberOfAirspaceAreas;i++)
	{
		if(AirspaceArea[i].Visible )
		{
			if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
			{
				//Start With Last Point on Polygon
				AngleSum = 0;
				FirstPoint = AirspaceArea[i].FirstPoint ; 
				LastPoint = AirspaceArea[i].FirstPoint + AirspaceArea[i].NumPoints - 1;

				XPoint = AirspacePoint[LastPoint].Screen.x; YPoint = AirspacePoint[LastPoint].Screen.y;
				Angle = Bearing(Lattitude, Longditude, AirspacePoint[LastPoint].Lattitude, AirspacePoint[LastPoint].Longditude );
				if(Angle > 180) Angle -= 360;	if(Angle < -180) Angle += 360;
				LastAngle = Angle;
				
				for(j=FirstPoint;j<=LastPoint;j++)
				{
					
					XPoint = AirspacePoint[j].Screen.x; YPoint = AirspacePoint[j].Screen.y;

					Angle = Bearing(Lattitude, Longditude, AirspacePoint[j].Lattitude, AirspacePoint[j].Longditude );
					if(Angle > 180) Angle -= 360;	if(Angle < -180) Angle += 360;
			
					NewAngle = Angle - LastAngle;  if(NewAngle > 180) NewAngle -= 360; if(NewAngle < -180) NewAngle += 360;
			
					AngleSum += NewAngle;  LastAngle = Angle;
				}

				if(AngleSum <0) AngleSum *= -1;

				if ((AngleSum > 358) && (AngleSum <362))
					return i;				
			}
		}
	}
	return -1;
}

BOOL CheckAirspaceAltitude(double Base, double Top)
{

	switch (AltitudeMode)
	{
		case ALLON : return TRUE;
		
		case CLIP : 
			if(Base < ClipAltitude)
				return TRUE;
			else
				return FALSE;

		case AUTO:
			if( ( GPS_INFO.Altitude > (Base - AltWarningMargin) ) && ( GPS_INFO.Altitude < (Top + AltWarningMargin) ))
				return TRUE;
			else
				return FALSE;

		case ALLBELOW:
			if(  (Base - AltWarningMargin) < GPS_INFO.Altitude )
				return  TRUE;
			else
				return FALSE;
	}
	return TRUE;
}