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
#include <aygshell.h>

#include <tchar.h>         

#include "resource.h"


#define  BINFILEMAGICNUMBER     0x4ab199f0
#define  BINFILEVERION          0x00000101
#define  BINFILEHEADER          "XCSoar Airspace File V1.0"

typedef struct{
  char     Header[32];
  DWORD    MagicNumber;
  DWORD    Version;
  FILETIME LastWriteSourceFile;
  DWORD    CrcSourceFile;          // not used at the moment
}BinFileHeader_t;


static bool StartsWith(TCHAR *Text, TCHAR *LookFor);
static void ReadCoords(TCHAR *Text, double *X, double *Y);
static void AddAirspaceCircle(AIRSPACE_AREA *Temp, double CenterX, double CenterY, double Radius);
static void AddPoint(AIRSPACE_POINT *Temp);
static void AddArea(AIRSPACE_AREA *Temp);
static void ReadAltitude(TCHAR *Text, AIRSPACE_ALT *Alt);
static void CalculateArc(TCHAR *Text);
static void CalculateSector(TCHAR *Text);
static void ParseError(TCHAR *Line);


static int GetNextLine(FILE *fp, TCHAR *Text);
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
static int LineCount;



/////////////////////////////


#if AIRSPACEUSEBINFILE > 0
// if file changed, don't load from binary, load from normal and then save it.

void SaveAirspaceBinary(FILETIME LastWrite) {

  HANDLE hFile;// = INVALID_HANDLE_VALUE;
  DWORD dwBytesRead;
  BinFileHeader_t Header;

  hFile = CreateFile(TEXT("xcsoar-airspace.bin"),
		     GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
		     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  strncpy(Header.Header, BINFILEHEADER, sizeof(Header.Header));
  Header.MagicNumber = BINFILEMAGICNUMBER;
  Header.Version = BINFILEVERION;
  Header.LastWriteSourceFile = LastWrite;
  Header.CrcSourceFile = 0;

  WriteFile(hFile, &Header,
	    sizeof(Header), &dwBytesRead, NULL);

  WriteFile(hFile, &NumberOfAirspaceAreas,
	    sizeof(unsigned int), &dwBytesRead, NULL);
  WriteFile(hFile, &NumberOfAirspacePoints,
	    sizeof(unsigned int), &dwBytesRead, NULL);
  WriteFile(hFile, &NumberOfAirspaceCircles,
	    sizeof(unsigned int), &dwBytesRead, NULL);

  WriteFile(hFile, AirspaceArea,
	    sizeof(AIRSPACE_AREA)*NumberOfAirspaceAreas,
	    &dwBytesRead, NULL);
  WriteFile(hFile, AirspacePoint,
	    sizeof(AIRSPACE_POINT)*NumberOfAirspacePoints,
	    &dwBytesRead, NULL);
  WriteFile(hFile, AirspaceCircle,
	    sizeof(AIRSPACE_CIRCLE)*NumberOfAirspaceCircles,
	    &dwBytesRead, NULL);

  CloseHandle(hFile);

}


bool LoadAirspaceBinary(FILETIME LastWrite) {
//  TCHAR szTemp[100]; // unused var RB

  HANDLE hFile;// = INVALID_HANDLE_VALUE;

  DWORD dwNumBytesRead;

  hFile = CreateFile(TEXT("xcsoar-airspace.bin"),
		     GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,
		     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

  if(hFile != INVALID_HANDLE_VALUE )

    {


      BinFileHeader_t Header;

      if ((ReadFile(hFile, &Header, sizeof(Header), &dwNumBytesRead, NULL) == 0)
          ||
          (dwNumBytesRead != sizeof(Header))
          ||
          (Header.MagicNumber != BINFILEMAGICNUMBER)
          ||
          (Header.Version != BINFILEVERION)
          ||
          (Header.LastWriteSourceFile.dwLowDateTime != LastWrite.dwLowDateTime)
          ||
          (Header.LastWriteSourceFile.dwHighDateTime != LastWrite.dwHighDateTime)
          ){

        CloseHandle(hFile);
        return(false);

      }


      HWND hProgress;

      hProgress = CreateProgressDialog(gettext(TEXT("Loading Airspace File...")));

      //wsprintf(szTemp,TEXT("0%%"));
      //SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);

      ReadFile(hFile,&NumberOfAirspaceAreas,
	       sizeof(unsigned int),&dwNumBytesRead,NULL);

      ReadFile(hFile,&NumberOfAirspacePoints,
	       sizeof(unsigned int),&dwNumBytesRead,NULL);

      ReadFile(hFile,&NumberOfAirspaceCircles,
	       sizeof(unsigned int),&dwNumBytesRead,NULL);

      if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
      if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
      if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);

      AirspaceArea  = (AIRSPACE_AREA *)
	LocalAlloc(LMEM_FIXED, NumberOfAirspaceAreas   * sizeof(AIRSPACE_AREA));
      AirspacePoint = (AIRSPACE_POINT *)
	LocalAlloc(LMEM_FIXED, NumberOfAirspacePoints  * sizeof(AIRSPACE_POINT));
      AirspaceCircle = (AIRSPACE_CIRCLE *)
	LocalAlloc(LMEM_FIXED, NumberOfAirspaceCircles * sizeof(AIRSPACE_CIRCLE));

      unsigned int i;

      StepProgressDialog();

      for (i=0; i<NumberOfAirspaceAreas; i++) {

	ReadFile(hFile,&AirspaceArea[i],
		 sizeof(AIRSPACE_AREA),
		 &dwNumBytesRead,NULL);
      }
      StepProgressDialog();
      StepProgressDialog();
      StepProgressDialog();

      for (i=0; i<NumberOfAirspacePoints; i++) {
	ReadFile(hFile,&AirspacePoint[i],
		 sizeof(AIRSPACE_POINT),
		 &dwNumBytesRead,NULL);
      }
      StepProgressDialog();
      StepProgressDialog();
      StepProgressDialog();

      for (i=0; i<NumberOfAirspaceCircles; i++) {
	ReadFile(hFile,&AirspaceCircle[i],
		 sizeof(AIRSPACE_CIRCLE),
		 &dwNumBytesRead,NULL);
      }
      StepProgressDialog();
      StepProgressDialog();
      StepProgressDialog();

      CloseHandle(hFile);

      //wsprintf(szTemp,TEXT("100%%"));
      //SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);

      return true;
    }
  return false; // couldn't find it...

return false;

}
#endif

///////////////////////////////

void CloseAirspace() {
  NumberOfAirspacePoints = 0;
  NumberOfAirspaceAreas = 0;
  NumberOfAirspaceCircles = 0;
  if(AirspaceArea != NULL)   {
    LocalFree((HLOCAL)AirspaceArea);
    AirspaceArea = NULL;
  }
  if(AirspacePoint != NULL)  {
    LocalFree((HLOCAL)AirspacePoint);
    AirspacePoint = NULL;
  }
  if(AirspaceCircle != NULL) {
    AirspaceCircle = NULL;
    LocalFree((HLOCAL)AirspaceCircle);
  }
}


// this can now be called multiple times to load several airspaces.
// to start afresh, call CloseAirspace()

void ReadAirspace(FILE *fp)
{
  int Mode = WAITING;
  int Tick = 0; int Tock=0;
  double fSize, fPos;
  DWORD dwPos;
  int OldNumberOfAirspacePoints = NumberOfAirspacePoints; 
  int OldNumberOfAirspaceAreas = NumberOfAirspaceAreas; 
  int OldNumberOfAirspaceCircles = NumberOfAirspaceCircles;
  int i;

  LineCount = 0;

  ReadMode = COUNT;

  HWND hProgress;

  hProgress=CreateProgressDialog(gettext(TEXT("Loading Airspace File...")));

  fSize = (double)GetFileSize((void *)_fileno(fp),NULL);

  TempArea.FirstPoint = 0;
  while(GetNextLine(fp, TempString))
    {
      Tock++; Tock %= 400;
      if(Tock == 0)
	{
	  dwPos = ftell(fp);
	  fPos = dwPos * 100;
	  fPos = fPos / (2*fSize);

    StepProgressDialog();

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

  // initialise the areas

  // old pointers, in case we have multiple airspace files
  AIRSPACE_CIRCLE* OldAirspaceCircle = AirspaceCircle;
  AIRSPACE_POINT* OldAirspacePoint = AirspacePoint;
  AIRSPACE_AREA* OldAirspaceArea = AirspaceArea;

  // allocate new memory
  AirspaceCircle = 
    (AIRSPACE_CIRCLE *)LocalAlloc(LMEM_FIXED, NumberOfAirspaceCircles * sizeof(AIRSPACE_CIRCLE));
  AirspacePoint = 
    (AIRSPACE_POINT *) LocalAlloc(LMEM_FIXED, NumberOfAirspacePoints  * sizeof(AIRSPACE_POINT));
  AirspaceArea  = 
    (AIRSPACE_AREA *)  LocalAlloc(LMEM_FIXED, NumberOfAirspaceAreas   * sizeof(AIRSPACE_AREA));

  // can't allocate memory, so delete everything
  if(( AirspaceCircle == NULL) || (AirspacePoint == NULL) || (AirspaceArea == NULL))
    {
      NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0; NumberOfAirspaceCircles = 0;
      if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
      if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
      if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);

      if(OldAirspaceArea != NULL)   LocalFree((HLOCAL)OldAirspaceArea);
      if(OldAirspacePoint != NULL)  LocalFree((HLOCAL)OldAirspacePoint);
      if(OldAirspaceCircle != NULL) LocalFree((HLOCAL)OldAirspaceCircle);

      return;
    }

  if (OldAirspaceCircle != NULL) {
    // copy old values into new
    for (i=0; i<OldNumberOfAirspaceCircles; i++) {
      memcpy(&AirspaceCircle[i],&OldAirspaceCircle[i],sizeof(AIRSPACE_CIRCLE));
    }
    // free the old values
    LocalFree((HLOCAL)OldAirspaceCircle);
  }

  if (OldAirspaceArea != NULL) {
    // copy old values into new
    for (i=0; i<OldNumberOfAirspaceAreas; i++) {
      memcpy(&AirspaceArea[i],&OldAirspaceArea[i],sizeof(AIRSPACE_AREA));
    }
    // free the old values
    LocalFree((HLOCAL)OldAirspaceArea);
  }

  if (OldAirspacePoint != NULL) {
    // copy old values into new
    for (i=0; i<OldNumberOfAirspacePoints; i++) {
      memcpy(&AirspacePoint[i],&OldAirspacePoint[i],sizeof(AIRSPACE_POINT));
    }
    // free the old values
    LocalFree((HLOCAL)OldAirspacePoint);
  }

  // ok, start the read

  Mode = WAITING;
  ReadMode = FILL;
  NumberOfAirspacePoints = OldNumberOfAirspacePoints; 
  NumberOfAirspaceAreas = OldNumberOfAirspaceAreas; 
  NumberOfAirspaceCircles = OldNumberOfAirspaceCircles;

  TempArea.FirstPoint = 0;
  fseek(fp, 0, SEEK_SET );
  LineCount = -1;

  while(GetNextLine(fp, TempString))
    {
      Tock++; Tock %= 400;
      if(Tock == 0)
	{
	  fPos = ftell(fp);
	  //fPos = LOWORD(fPos);
	  fPos *= 100;
	  fPos = fPos / (2*fSize);
	  fPos += 50;
	  //wsprintf(szTemp,TEXT("%d%%"),int(fPos));	
	  //SetDlgItemText(hProgress,IDC_PROGRESS,szTemp);

          StepProgressDialog();

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

}


int GetNextLine(FILE *fp, TCHAR *Text)
{
  TCHAR *Comment;
  int size;

  while(ReadStringX(fp, 200, Text))
    {
      LineCount++;
      if(_tcslen(Text) <=2)
	{
	}
      else if(StartsWith(Text,TEXT("AT ")))
	{
	}
      else if(StartsWith(Text,TEXT("SP ")))
	{
	}
      else if(StartsWith(Text,TEXT("SB ")))
	{
	}
      else if(StartsWith(Text,TEXT("V Z=")))
	{
	}
      else if(StartsWith(Text,TEXT("TC ")))
	{
	}
      else if(StartsWith(Text,TEXT("TO ")))
	{
	}
      else if(StartsWith(Text,TEXT("TYPE")))
	{
	}
      else if(!StartsWith(Text,TEXT("V T=1")))
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
      else if(StartsWith(&Text[3],TEXT("E"))) Temp->Type = CLASSE;
      else if(StartsWith(&Text[3],TEXT("F"))) Temp->Type = CLASSF;
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
      else if(StartsWith(&Text[3],TEXT("E"))) Temp->Type = CLASSE;
      else if(StartsWith(&Text[3],TEXT("F"))) Temp->Type = CLASSF;
      else Temp->Type = OTHER;

      Rotation = 1;

      return COMMAND_AC;
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
      else if(StartsWith(&Text[3],TEXT("E"))) Temp->Type = CLASSE;
      else if(StartsWith(&Text[3],TEXT("F"))) Temp->Type = CLASSF;
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
	

static bool StartsWith(TCHAR *Text, TCHAR *LookFor)
{
  while(1) {
    if (!(*LookFor)) return TRUE;
    if (*Text != *LookFor) return FALSE;
    Text++; LookFor++;
  }
  /*
  if(_tcsstr(Text,LookFor) == Text)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }
  */
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

  if (LineCount == -1)                                      // -1 means we are in the second pass
    return;                                                 // all warnings are still displayed

  TCHAR Message[200];

  wsprintf(Message,TEXT("Error in Section %s at Line %d Text %s"),TempArea.Name,LineCount,Line);

  MessageBox(hWndMainWindow,Message,TEXT("Error in Airspace File"),MB_OK|MB_ICONINFORMATION);
}



void FindAirspaceAreaBounds() {
  unsigned i, j;
  for(i=0; i<NumberOfAirspaceAreas; i++) {
    bool first = true;

    for(j= AirspaceArea[i].FirstPoint; 
        j< AirspaceArea[i].FirstPoint+AirspaceArea[i].NumPoints; j++) {
      
      if (first) {
        AirspaceArea[i].bounds.minx = AirspacePoint[j].Longditude;
        AirspaceArea[i].bounds.maxx = AirspacePoint[j].Longditude;
        AirspaceArea[i].bounds.miny = AirspacePoint[j].Lattitude;
        AirspaceArea[i].bounds.maxy = AirspacePoint[j].Lattitude;
        first = false;
      } else {
        AirspaceArea[i].bounds.minx = min(AirspacePoint[j].Longditude,
                                          AirspaceArea[i].bounds.minx);
        AirspaceArea[i].bounds.maxx = max(AirspacePoint[j].Longditude,
                                          AirspaceArea[i].bounds.maxx);
        AirspaceArea[i].bounds.miny = min(AirspacePoint[j].Lattitude,
                                          AirspaceArea[i].bounds.miny);
        AirspaceArea[i].bounds.maxy = max(AirspacePoint[j].Lattitude,
                                          AirspaceArea[i].bounds.maxy);
      }


    }
  }



}


extern TCHAR szRegistryAirspaceFile[];
extern TCHAR szRegistryAdditionalAirspaceFile[];

void ReadAirspace(void)
{
  TCHAR	szFile1[MAX_PATH] = TEXT("\0");
  TCHAR	szFile2[MAX_PATH] = TEXT("\0");
	
  FILE *fp;
  FILE *fp2;
  FILETIME LastWriteTime;
  FILETIME LastWriteTime2;

  GetRegistryString(szRegistryAirspaceFile, szFile1, MAX_PATH);
  GetRegistryString(szRegistryAdditionalAirspaceFile, szFile2, MAX_PATH);


  fp  = _tfopen(szFile1, TEXT("rt"));
  fp2 = _tfopen(szFile2, TEXT("rt"));

  if (fp != NULL){

    GetFileTime((void *)_fileno(fp), NULL, NULL, &LastWriteTime);

    if (fp2 != NULL) {
      GetFileTime((void *)_fileno(fp2), NULL, NULL, &LastWriteTime2);
      if (LastWriteTime2.dwHighDateTime>
          LastWriteTime.dwHighDateTime) {
        // this file is newer, so use it as the time stamp
        LastWriteTime = LastWriteTime2;
      }
    }

    if (AIRSPACEFILECHANGED
        #if AIRSPACEUSEBINFILE > 0
        ||!LoadAirspaceBinary(LastWriteTime)
        #else
        || (true)
        #endif
      ) {

      ReadAirspace(fp);

      // also read any additional airspace
      if (fp2 != NULL) {
        ReadAirspace(fp2);
      }
      #if AIRSPACEUSEBINFILE > 0
      SaveAirspaceBinary(LastWriteTime);
      #endif
    }

  	fclose(fp);

  }

  if (fp2 != NULL) {
    fclose(fp2);
  }

  FindAirspaceAreaBounds();

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

double fastBearing(double x0, double y0, double x1, double y1) {
  double dx = x1-x0;
  double dy = y1-y0;
  return atan2(dy,dx)*RAD_TO_DEG;
}





//
// JMW this routine checks if the polygon is closed around the test point
// by integrating the angles from the test point to each polygon point
//
// There must be faster ways of doing this without trigonometry
//

int FindAirspaceAreaOld(double Longditude,double Lattitude)
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
	      Angle = fastBearing(Lattitude, Longditude, AirspacePoint[LastPoint].Lattitude, AirspacePoint[LastPoint].Longditude );
	      LastAngle = Angle;
				
	      for(j=FirstPoint;j<=LastPoint;j++)
		{
					
		  XPoint = AirspacePoint[j].Screen.x; YPoint = AirspacePoint[j].Screen.y;

		  Angle = fastBearing(Lattitude, Longditude, AirspacePoint[j].Lattitude, AirspacePoint[j].Longditude );
			
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


///////////////////////////////////////////////////

// Copyright 2001, softSurfer (www.softsurfer.com)
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.

//    a Point is defined by its coordinates {int x, y;}
//===================================================================

// isLeft(): tests if a point is Left|On|Right of an infinite line.
//    Input:  three points P0, P1, and P2
//    Return: >0 for P2 left of the line through P0 and P1
//            =0 for P2 on the line
//            <0 for P2 right of the line
//    See: the January 2001 Algorithm "Area of 2D and 3D Triangles and Polygons"
inline double
isLeft( AIRSPACE_POINT P0, AIRSPACE_POINT P1, AIRSPACE_POINT P2 )
{
    return ( (P1.Longditude - P0.Longditude) * (P2.Lattitude - P0.Lattitude)
            - (P2.Longditude - P0.Longditude) * (P1.Lattitude - P0.Lattitude) );
}
//===================================================================

// cn_PnPoly(): crossing number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  0 = outside, 1 = inside
// This code is patterned after [Franklin, 2000]
int
cn_PnPoly( AIRSPACE_POINT P, AIRSPACE_POINT* V, int n )
{
    int    cn = 0;    // the crossing number counter

    // loop through all edges of the polygon

    for (int i=0; i<n; i++) {    
      // edge from V[i] to V[i+1]

       if (((V[i].Lattitude <= P.Lattitude) 
            && (V[i+1].Lattitude > P.Lattitude))    // an upward crossing
        || ((V[i].Lattitude > P.Lattitude) 
            && (V[i+1].Lattitude <= P.Lattitude))) { // a downward crossing

            // compute the actual edge-ray intersect x-coordinate
         float vt = (float)((P.Lattitude - V[i].Lattitude) 
                            / (V[i+1].Lattitude - V[i].Lattitude));

            if (P.Longditude < V[i].Longditude
                + vt * (V[i+1].Longditude - V[i].Longditude)) // P.x < intersect
                ++cn;   // a valid crossing of y=P.Lattitude right of P.Longditude
        }
    }
    return (cn&1);    // 0 if even (out), and 1 if odd (in)

}
//===================================================================

// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only if P is outside V[])
int
wn_PnPoly( AIRSPACE_POINT P, AIRSPACE_POINT* V, int n )
{
    int    wn = 0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<n; i++) {   // edge from V[i] to V[i+1]
        if (V[i].Lattitude <= P.Lattitude) {         // start y <= P.Lattitude
            if (V[i+1].Lattitude > P.Lattitude)      // an upward crossing
                if (isLeft( V[i], V[i+1], P) > 0)  // P left of edge
                    ++wn;            // have a valid up intersect
        }
        else {                       // start y > P.Lattitude (no test needed)
            if (V[i+1].Lattitude <= P.Lattitude)     // a downward crossing
                if (isLeft( V[i], V[i+1], P) < 0)  // P right of edge
                    --wn;            // have a valid down intersect
        }
    }
    return wn;
}
//===================================================================



int FindAirspaceArea(double Longditude,double Lattitude)
{
  unsigned i;

  if(NumberOfAirspaceAreas == 0)
    {
      return -1;
    }

  AIRSPACE_POINT thispoint;

  thispoint.Longditude = Longditude;
  thispoint.Lattitude = Lattitude;

  for(i=0;i<NumberOfAirspaceAreas;i++)
    {
      if(AirspaceArea[i].Visible ) 
        // JMW is this a bug?
        // surely we should check it whether it is visible or not
        // in almost all cases it will be, so ok.
	{
	  if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude, AirspaceArea[i].Top.Altitude))
	    {

              // first check if point is within bounding box
              if (
                  (Lattitude> AirspaceArea[i].bounds.miny)&&
                  (Lattitude< AirspaceArea[i].bounds.maxy)&&
                  (Longditude> AirspaceArea[i].bounds.minx)&&
                  (Longditude< AirspaceArea[i].bounds.maxx)
                  )
                {
                  // it is within, so now do detailed polygon test
                  if (wn_PnPoly(thispoint,
                                &AirspacePoint[AirspaceArea[i].FirstPoint],
                                AirspaceArea[i].NumPoints-1) != 0) {
                    // we are inside the i'th airspace area
                    return i;
                  }
                }
	    }
	}
    }
  // not inside any airspace
  return -1;
}

