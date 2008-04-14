/*

Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

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

}

*/

#include "stdafx.h"
#include "Airspace.h"
#include "externs.h"
#include "dialogs.h"
#include "Utils.h"
#include "XCSoar.h"
#include "MapWindow.h"
#include "RasterTerrain.h"
#include <windows.h>
#include <Commctrl.h>
#include <math.h>
#include <aygshell.h>

#include <tchar.h>
#include <ctype.h>

#include "resource.h"
#include "wcecompat/ts_string.h"


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

static int AirspacePointSize;

void DumpAirspaceFile(void);

static bool StartsWith(TCHAR *Text, const TCHAR *LookFor);
static bool ReadCoords(TCHAR *Text, double *X, double *Y);
static void AddAirspaceCircle(AIRSPACE_AREA *Temp, double CenterX, double CenterY, double Radius);
static void AddPoint(AIRSPACE_POINT *Temp, unsigned *AeraPointCount);
static void AddArea(AIRSPACE_AREA *Temp);
static void ReadAltitude(TCHAR *Text, AIRSPACE_ALT *Alt);
static void CalculateArc(TCHAR *Text);
static void CalculateSector(TCHAR *Text);
static bool ParseLine(int nLineType);


static int GetNextLine(ZZIP_FILE *fp, TCHAR *Text);

static bool bFillMode = false;
static bool	bWaiting = true;

static TCHAR TempString[READLINE_LENGTH+1];

static AIRSPACE_AREA TempArea;
static AIRSPACE_POINT TempPoint;
static int Rotation = 1;
static double CenterX = 0;
static double CenterY = 0;
static float Radius = 0;
static float Width = 0;
static float Zoom = 0;
static int LineCount;
static double lastQNH;

int AirspacePriority[AIRSPACECLASSCOUNT];

static const int k_nLineTypes = 9;

static const int k_nLtAC	= 0;
static const int k_nLtAN	= 1;
static const int k_nLtAL	= 2;
static const int k_nLtAH	= 3;
static const int k_nLtV		= 4;
static const int k_nLtDP	= 5;
static const int k_nLtDB	= 6;
static const int k_nLtDA	= 7;
static const int k_nLtDC	= 8;

static const int k_nAreaCount = 12;
static const TCHAR* k_strAreaStart[k_nAreaCount] = {
					_T("R"),
					_T("Q"),
					_T("P"),
					_T("A"),
					_T("B"),
					_T("C"),
					_T("CTR"),
					_T("D"),
					_T("GP"),
					_T("W"),
					_T("E"),
					_T("F")
};
static const int k_nAreaType[k_nAreaCount] = {
					RESTRICT,
					DANGER,
					PROHIBITED,
					CLASSA,
					CLASSB,
					CLASSC,
					CTR,
					CLASSD,
					NOGLIDER,
					WAVE,
					CLASSE,
					CLASSF};

/////////////////////////////


bool CheckAirspacePoint(int Idx){
  if (Idx < 0 || Idx >= AirspacePointSize){
    return false;
    //throw "Airspace Parser: Memory access error!";
  }
  return true;
}


bool ValidAirspace(void) {
  return (NumberOfAirspacePoints>0)||(NumberOfAirspaceAreas>0)||(NumberOfAirspaceCircles>0);
}

///////////////////////////////

void CloseAirspace() {

  AirspaceWarnListClear();

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
  if(AirspaceScreenPoint != NULL)  {
    LocalFree((HLOCAL)AirspaceScreenPoint);
    AirspaceScreenPoint = NULL;
  }
  if(AirspaceCircle != NULL) {
    AirspaceCircle = NULL;
    LocalFree((HLOCAL)AirspaceCircle);
  }
}

// this can now be called multiple times to load several airspaces.
// to start afresh, call CloseAirspace()

void ReadAirspace(ZZIP_FILE *fp)
{
  StartupStore(TEXT("ReadAirspace\n"));

  int	Tock = 0;
  DWORD	dwStep;
  DWORD	dwPos;
  DWORD	dwOldPos = 0L;
  int	i;
  int	nLineType;
  int	OldNumberOfAirspacePoints  = NumberOfAirspacePoints;
  int	OldNumberOfAirspaceAreas   = NumberOfAirspaceAreas;
  int	OldNumberOfAirspaceCircles = NumberOfAirspaceCircles;

  int	NumberOfAirspacePointsPass[2];
  int	NumberOfAirspaceAreasPass[2];
  int	NumberOfAirspaceCirclesPass[2];

  LineCount = 0;

  lastQNH = QNH;

  CreateProgressDialog(gettext(TEXT("Loading Airspace File...")));
  // Need step size finer than default 10
  SetProgressStepSize(5);
  dwStep = zzip_file_size(fp) / 10L;

  TempArea.FirstPoint = NumberOfAirspacePoints;	// JG 10-Nov-2005

  bFillMode = false;
  bWaiting = true;
  StepProgressDialog();
  while((nLineType = GetNextLine(fp, TempString)) >= 0)
  {
    Tock++;
    Tock %= 50;
    if(Tock == 0)
      {
        dwPos = zzip_tell(fp);
        if ((dwPos - dwOldPos) >= dwStep)
          {
            StepProgressDialog();
            dwOldPos = dwPos;
          }
      }

    if (!ParseLine(nLineType)){
      CloseAirspace();
      return;
    }
  }

  // Process final area (if any). bFillMode is false.  JG 10-Nov-2005
  if (!bWaiting)
    NumberOfAirspaceAreas++;    // ????

  NumberOfAirspacePointsPass[0] = NumberOfAirspacePoints - OldNumberOfAirspacePoints;
  NumberOfAirspaceAreasPass[0] = NumberOfAirspaceAreas - OldNumberOfAirspaceAreas;
  NumberOfAirspaceCirclesPass[0] = NumberOfAirspaceCircles - OldNumberOfAirspaceCircles;

  // initialise the areas

  // old pointers, in case we have multiple airspace files
  AIRSPACE_CIRCLE* OldAirspaceCircle = AirspaceCircle;
  AIRSPACE_POINT*  OldAirspacePoint = AirspacePoint;
  POINT*  OldAirspaceScreenPoint = AirspaceScreenPoint;
  AIRSPACE_AREA*   OldAirspaceArea = AirspaceArea;

  // allocate new memory
  AirspaceCircle = (AIRSPACE_CIRCLE *)LocalAlloc(LMEM_FIXED,
                    NumberOfAirspaceCircles * sizeof(AIRSPACE_CIRCLE));

  AirspacePoint  = (AIRSPACE_POINT *)LocalAlloc(LMEM_FIXED,
                                                (NumberOfAirspacePoints)
                                                * sizeof(AIRSPACE_POINT));

  AirspacePointSize = NumberOfAirspacePoints;//  * sizeof(AIRSPACE_POINT);

  AirspaceScreenPoint  = (POINT *)LocalAlloc(LMEM_FIXED,
					     NumberOfAirspacePoints
					     * sizeof(POINT));


  AirspaceArea   = (AIRSPACE_AREA *)  LocalAlloc(LMEM_FIXED,
                                                 NumberOfAirspaceAreas
                                                 * sizeof(AIRSPACE_AREA));

  // can't allocate memory, so delete everything
  if(( AirspaceCircle == NULL) || (AirspacePoint == NULL)
     || (AirspaceArea == NULL) || (AirspaceScreenPoint == NULL))
    {
      NumberOfAirspacePoints = 0; NumberOfAirspaceAreas = 0;
      NumberOfAirspaceCircles = 0;
      if(AirspaceArea != NULL)   LocalFree((HLOCAL)AirspaceArea);
      if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);
      if(AirspaceScreenPoint != NULL)  LocalFree((HLOCAL)AirspaceScreenPoint);
      if(AirspaceCircle != NULL) LocalFree((HLOCAL)AirspaceCircle);

      if(OldAirspaceArea != NULL)   LocalFree((HLOCAL)OldAirspaceArea);
      if(OldAirspacePoint != NULL)  LocalFree((HLOCAL)OldAirspacePoint);
      if(OldAirspaceCircle != NULL) LocalFree((HLOCAL)OldAirspaceCircle);
      if(OldAirspaceScreenPoint != NULL) LocalFree((HLOCAL)OldAirspaceScreenPoint);

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

  if (OldAirspaceScreenPoint != NULL) {
    LocalFree((HLOCAL)OldAirspaceScreenPoint);
  }

  // ok, start the read
  NumberOfAirspacePoints  = OldNumberOfAirspacePoints;
  NumberOfAirspaceAreas	  = OldNumberOfAirspaceAreas;
  NumberOfAirspaceCircles = OldNumberOfAirspaceCircles;

  TempArea.FirstPoint = NumberOfAirspacePoints;	// JG 10-Nov-2005
  zzip_seek(fp, 0, SEEK_SET );
  LineCount = -1;

  bFillMode = true;
  bWaiting = true;
  dwOldPos	= 0L;
  StepProgressDialog();
  CenterY = CenterX = 0;
  Rotation = 1;

  while((nLineType = GetNextLine(fp, TempString)) >= 0)
  {
    Tock++;
    Tock %= 50;
    if(Tock == 0)
      {
        dwPos = zzip_tell(fp);
        if ((dwPos - dwOldPos) >= dwStep)
          {
            StepProgressDialog();
            dwOldPos = dwPos;
          }
      }

    ParseLine(nLineType);
  }

  // Process final area (if any). bFillMode is true.  JG 10-Nov-2005
  if (!bWaiting)
    AddArea(&TempArea);

  NumberOfAirspacePointsPass[1] = NumberOfAirspacePoints - OldNumberOfAirspacePoints;
  NumberOfAirspaceAreasPass[1] = NumberOfAirspaceAreas - OldNumberOfAirspaceAreas;
  NumberOfAirspaceCirclesPass[1] = NumberOfAirspaceCircles - OldNumberOfAirspaceCircles;

  if (NumberOfAirspacePointsPass[0] != NumberOfAirspacePointsPass[1]
      || NumberOfAirspaceAreasPass[0] != NumberOfAirspaceAreasPass[1]
      || NumberOfAirspaceCirclesPass[0] != NumberOfAirspaceCirclesPass[1]){

    if (MessageBoxX(NULL,
                    gettext(TEXT("Internal Airspace Parser Error!\r\nPlease send this Airspacefile to Support")),
                    gettext(TEXT("Airspace")), MB_OKCANCEL) == IDCANCEL) {
    }

  }

#ifdef DEBUG
  // only do this if debugging
  DumpAirspaceFile();
#endif

//  if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);

}

static bool ParseLine(int nLineType)
{
  int		nIndex;

  switch (nLineType)
    {
    case k_nLtAC:
      if (bFillMode)
        {
          if (!bWaiting)
            AddArea(&TempArea);
          TempArea.NumPoints = 0;
          TempArea.Type = OTHER;
          for (nIndex = 0; nIndex < k_nAreaCount; nIndex++)
            {
              if (StartsWith(&TempString[3], k_strAreaStart[nIndex]))
                {
                  TempArea.Type = k_nAreaType[nIndex];
                  break;
                }
            }
          Rotation = +1;
        }
      else if (!bWaiting)   // Don't count circles JG 10-Nov-2005
        NumberOfAirspaceAreas++;

      Rotation = +1;
      bWaiting = false;
      break;

    case k_nLtAN:
      if (bFillMode)
        {
          TempString[NAME_SIZE] = '\0';
          _tcscpy(TempArea.Name, &TempString[3]);
        }
      break;

    case k_nLtAL:
      if (bFillMode)
        ReadAltitude(&TempString[3], &TempArea.Base);
      break;

    case k_nLtAH:
      if (bFillMode)
        ReadAltitude(&TempString[3],&TempArea.Top);
      break;

    case k_nLtV:
      // Need to set these while in count mode, or DB/DA will crash
      if(StartsWith(&TempString[2], TEXT("X=")) || StartsWith(&TempString[2], TEXT("x=")))
        {
          if (ReadCoords(&TempString[4],&CenterX, &CenterY))
            break;
        }
      else if(StartsWith(&TempString[2],TEXT("D=-")) || StartsWith(&TempString[2],TEXT("d=-")))
        {
          Rotation = -1;
          break;
        }
      else if(StartsWith(&TempString[2],TEXT("D=+")) || StartsWith(&TempString[2],TEXT("d=+")))
        {
          Rotation = +1;
          break;
        }
      else if(StartsWith(&TempString[2],TEXT("Z")) || StartsWith(&TempString[2],TEXT("z")))
        {
          // ToDo Display Zool Level
          break;
        }
      else if(StartsWith(&TempString[2],TEXT("W")) || StartsWith(&TempString[2],TEXT("w")))
        {
          // ToDo width of an airway
          break;
        }
      else if(StartsWith(&TempString[2],TEXT("T")) || StartsWith(&TempString[2],TEXT("t")))
        {
          // ----- JMW THIS IS REQUIRED FOR LEGACY FILES
          break;
        }

      goto OnError;

    case k_nLtDP:
      /*
        if (bFillMode)
        {
			ReadCoords(&TempString[3],&TempPoint.Longitude ,
                                   &TempPoint.Latitude );
			AddPoint(&TempPoint);
		}
		else
			NumberOfAirspacePoints++;
    */
//		if (bFillMode)
      if (!ReadCoords(&TempString[3],&TempPoint.Longitude ,
                      &TempPoint.Latitude))
        goto OnError;
      AddPoint(&TempPoint, &TempArea.NumPoints);
      // TempArea.NumPoints++;
      break;

    case k_nLtDB:
      CalculateArc(TempString);
      break;

    case k_nLtDA:
      CalculateSector(TempString);
      break;

    case k_nLtDC:
      if (bFillMode)
        {
          Radius = (float)StrToDouble(&TempString[2],NULL);
          Radius = (float)(Radius * NAUTICALMILESTOMETRES);
          AddAirspaceCircle(&TempArea, CenterX, CenterY, Radius);
        }
      else
        NumberOfAirspaceCircles++;

      bWaiting = true;
      break;

    default:
      break;
    }

  return(true);

OnError:

  if (!bFillMode){
    TCHAR sTmp[MAX_PATH];
    wsprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
             gettext(TEXT("Parse Error at Line")),
             LineCount, TempString,
             gettext(TEXT("Line skipped.")));
    if (MessageBoxX(hWndMainWindow, sTmp, gettext(TEXT("Airspace")),
                    MB_OKCANCEL) == IDCANCEL){
      return(false);
    }

  }

  return(true);

}


// Returns index of line type found, or -1 if end of file reached
static int GetNextLine(ZZIP_FILE *fp, TCHAR *Text)
{
  TCHAR	*Comment;
  int		nSize;
  int		nLineType = -1;
  TCHAR sTmp[READLINE_LENGTH];

  while (ReadString(fp, READLINE_LENGTH, Text)){
    // JMW was ReadStringX

    LineCount++;

    nSize = _tcsclen(Text);

    // Ignore lines less than 3 characters
    // or starting with comment char
    if((nSize < 3) || (Text[0] == _T('*')))
      continue;

    // build a upercase copy of the tags
    _tcsncpy(sTmp, Text, sizeof(sTmp)/sizeof(sTmp[0]));
    sTmp[sizeof(sTmp)/sizeof(sTmp[0])-1] = '\0';
    _tcsupr(sTmp);

    // Only return expected lines
    switch (sTmp[0])
      {
      case _T('A'):
        switch (sTmp[1])
          {
          case _T('C'):
            nLineType = k_nLtAC;
            break;

          case _T('N'):
            nLineType = k_nLtAN;
            break;

          case _T('L'):
            nLineType = k_nLtAL;
            break;

          case _T('H'):
            nLineType = k_nLtAH;
            break;

          case _T('T'): // ignore airspace lables
            // ToDo: adding airspace labels
            continue;

          default:
            if (bFillMode){
              wsprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
                       gettext(TEXT("Parse Error at Line")),
                       LineCount, TempString,
                       gettext(TEXT("Line skipped.")));
              if (MessageBoxX(NULL, sTmp,
                              gettext(TEXT("Airspace")),
                              MB_OKCANCEL) == IDCANCEL)
                return(-1);
            }
            continue;
          }

        break;

      case _T('D'):
        switch (sTmp[1])
          {
          case _T('A'):
            nLineType = k_nLtDA;
            break;

          case _T('B'):
            nLineType = k_nLtDB;
            break;

          case _T('C'):
            nLineType = k_nLtDC;
            break;

          case _T('P'):
            nLineType = k_nLtDP;
            break;

            // todo DY airway segment
            // what about 'V T=' ?

          default:
            if (bFillMode){
              wsprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
                       gettext(TEXT("Parse Error at Line")),
                       LineCount, TempString,
                       gettext(TEXT("Line skipped.")));
              if (MessageBoxX(NULL, sTmp,
                              gettext(TEXT("Airspace")),
                              MB_OKCANCEL) == IDCANCEL)
                return(-1);
            }
            continue;
          }

        break;

      case _T('V'):
        nLineType = k_nLtV;
        break;

      case _T('S'):  // ignore the SB,SP ...
        if (sTmp[1] == _T('B'))
          continue;
        if (sTmp[1] == _T('P'))
          continue;

      default:
        if (bFillMode){
          wsprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
                   gettext(TEXT("Parse Error at Line")),
                   LineCount, TempString,
                   gettext(TEXT("Line skipped.")));
          if (MessageBoxX(NULL, sTmp, gettext(TEXT("Airspace")),
                          MB_OKCANCEL) == IDCANCEL)
            return(-1);
        }
        continue;
      }

    if (nLineType >= 0)		// Valid line found
      {
        // Strip comments and newline chars from end of line
        Comment = _tcschr(Text, _T('*'));
        if(Comment != NULL)
          {
            *Comment = _T('\0');		// Truncate line
            nSize = Comment - Text;		// Reset size
            if (nSize < 3)
              continue;				// Ensure newline removal won't fail
          }
        if(Text[nSize-1] == _T('\n'))
          Text[--nSize] = _T('\0');
        if(Text[nSize-1] == _T('\r'))
          Text[--nSize] = _T('\0');

        break;
      }
  }

  return nLineType;
}


static bool StartsWith(TCHAR *Text, const TCHAR *LookFor)
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

static bool ReadCoords(TCHAR *Text, double *X, double *Y)
{
  double Ydeg=0, Ymin=0, Ysec=0;
  double Xdeg=0, Xmin=0, Xsec=0;
  TCHAR *Stop;

  // ToDo, add more error checking and making it more tolerant/robust

  Ydeg = (double)StrToDouble(Text, &Stop);
  if ((Text == Stop) || (*Stop =='\0')) goto OnError;
  Stop++;
  Ymin = (double)StrToDouble(Stop, &Stop);
  if (Ymin<0 || Ymin >=60){
    // ToDo
  }
  if (*Stop =='\0') goto OnError;
  if(*Stop == ':'){
    Stop++;
    if (*Stop =='\0')
      goto OnError;
    Ysec = (double)StrToDouble(Stop, &Stop);
    if (Ysec<0 || Ysec >=60) {
      // ToDo
    }
  }

  *Y = Ysec/3600 + Ymin/60 + Ydeg;

  if (*Stop == ' ')
    Stop++;

  if (*Stop =='\0') goto OnError;
  if((*Stop == 'S') || (*Stop == 's'))
    {
      *Y = *Y * -1;
    }
  Stop++;
  if (*Stop =='\0') goto OnError;

  Xdeg = (double)StrToDouble(Stop, &Stop);
  Stop++;
  Xmin = (double)StrToDouble(Stop, &Stop);
  if(*Stop == ':'){
    Stop++;
    if (*Stop =='\0')
      goto OnError;
    Xsec = (double)StrToDouble(Stop, &Stop);
  }

  *X = Xsec/3600 + Xmin/60 + Xdeg;

  if (*Stop == ' ')
    Stop++;
  if (*Stop =='\0') goto OnError;
  if((*Stop == 'W') || (*Stop == 'w'))
    {
      *X = *X * -1;
    }

  if (*X<-180) {
    *X+= 360;
  }
  if (*X>180) {
    *X-= 360;
  }

  return(true);

OnError:
  return(false);

}


static void AirspaceAGLLookup(AIRSPACE_ALT *Top, AIRSPACE_ALT *Base,
			      double av_lat, double av_lon) {
  if (((Base->Base == abAGL) || (Top->Base == abAGL))) {

    RasterTerrain::Lock();
    // want most accurate rounding here
    RasterTerrain::SetTerrainRounding(0,0);

    double th =
      RasterTerrain::GetTerrainHeight(av_lat, av_lon);

    if (Base->Base == abAGL) {
      if (Base->AGL>=0) {
	Base->Altitude = Base->AGL+th;
      } else {
	// surface, set to zero
	Base->AGL = 0;
	Base->Altitude = 0;
      }
    }
    if (Top->Base == abAGL) {
      if (Top->AGL>=0) {
	Top->Altitude = Top->AGL+th;
      } else {
	// surface, set to zero
	Top->AGL = 0;
	Top->Altitude = 0;
      }
    }
    // JMW TODO: complain if out of terrain range (th<0)
    RasterTerrain::Unlock();
  }
}


static void AddAirspaceCircle(AIRSPACE_AREA *Temp,
                              double CenterX, double CenterY, double Radius)
{
  AIRSPACE_CIRCLE *NewCircle = NULL;

  if(!bFillMode)
    {
      NumberOfAirspaceCircles++;
    }
  else
    {
      NewCircle =  &AirspaceCircle[NumberOfAirspaceCircles];
      NumberOfAirspaceCircles++;

      _tcscpy(NewCircle->Name , Temp->Name);
      NewCircle->Latitude = CenterY;
      NewCircle->Longitude = CenterX;
      NewCircle->Radius = Radius;
      NewCircle->Type = Temp->Type;
      NewCircle->Top.Altitude  = Temp->Top.Altitude ;
      NewCircle->Top.FL   = Temp->Top.FL;
      NewCircle->Top.Base   = Temp->Top.Base;
      NewCircle->Top.AGL = Temp->Top.AGL;
      NewCircle->Base.Altitude  = Temp->Base.Altitude;
      NewCircle->Base.FL   = Temp->Base.FL;
      NewCircle->Base.Base   = Temp->Base.Base;
      NewCircle->Base.AGL = Temp->Base.AGL;
      NewCircle->Ack.AcknowledgedToday = false;
      NewCircle->Ack.AcknowledgementTime = 0;
      NewCircle->_NewWarnAckNoBrush = false;

      AirspaceAGLLookup(&NewCircle->Base, &NewCircle->Top,
			NewCircle->Latitude,
			NewCircle->Longitude);

    }
}

static void AddPoint(AIRSPACE_POINT *Temp, unsigned *AreaPointCount)
{
  AIRSPACE_POINT *NewPoint = NULL;

  /*
  if(!bFillMode)
    {
      NumberOfAirspacePoints++;
    }
  else
    {

      CheckAirspacePoint(NumberOfAirspacePoints);

      NewPoint = &AirspacePoint[NumberOfAirspacePoints];
      NumberOfAirspacePoints++;

      NewPoint->Latitude  = Temp->Latitude;
      NewPoint->Longitude = Temp->Longitude;
    }
  */

  ASSERT(Temp != NULL);
  ASSERT(AreaPointCount != NULL);

  if(bFillMode){

    CheckAirspacePoint(NumberOfAirspacePoints);

    NewPoint = &AirspacePoint[NumberOfAirspacePoints];

    NewPoint->Latitude  = Temp->Latitude;
    NewPoint->Longitude = Temp->Longitude;

    (*AreaPointCount)++;

  }

  NumberOfAirspacePoints++;

}

static void AddArea(AIRSPACE_AREA *Temp)
{
  AIRSPACE_AREA *NewArea = NULL;
  AIRSPACE_POINT *PointList = NULL;
  unsigned i;


  if(!bFillMode)
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
      NewArea->Base.Base   = Temp->Base.Base;
      NewArea->Base.AGL   = Temp->Base.AGL  ;
      NewArea->NumPoints = Temp->NumPoints;
      NewArea->Top.Altitude  = Temp->Top.Altitude ;
      NewArea->Top.FL = Temp->Top.FL;
      NewArea->Top.Base   = Temp->Top.Base;
      NewArea->Top.AGL  = Temp->Top.AGL ;
      NewArea->FirstPoint = Temp->FirstPoint;
      NewArea->Ack.AcknowledgedToday = false;
      NewArea->Ack.AcknowledgementTime = 0;
      NewArea->_NewWarnAckNoBrush = false;

      Temp->FirstPoint = Temp->FirstPoint + Temp->NumPoints ;

      if (Temp->NumPoints > 0) {

        CheckAirspacePoint(NewArea->FirstPoint);

        PointList = &AirspacePoint[NewArea->FirstPoint];
        NewArea->MaxLatitude = -90;
        NewArea->MinLatitude = 90;
        NewArea->MaxLongitude  = -180;
        NewArea->MinLongitude  = 180;

        for(i=0;i<Temp->NumPoints; i++)
        {
          if(PointList[i].Latitude > NewArea->MaxLatitude)
            NewArea->MaxLatitude = PointList[i].Latitude ;
          if(PointList[i].Latitude < NewArea->MinLatitude)
            NewArea->MinLatitude = PointList[i].Latitude ;

          if(PointList[i].Longitude  > NewArea->MaxLongitude)
            NewArea->MaxLongitude  = PointList[i].Longitude ;
          if(PointList[i].Longitude  < NewArea->MinLongitude)
            NewArea->MinLongitude  = PointList[i].Longitude ;
        }
	AirspaceAGLLookup(&NewArea->Base, &NewArea->Top,
			  (NewArea->MaxLatitude+NewArea->MinLatitude)/2,
			  (NewArea->MaxLongitude+NewArea->MinLongitude)/2);

      } else {

        NewArea->MaxLatitude = 0;
        NewArea->MinLatitude = 0;
        NewArea->MaxLongitude  = 0;
        NewArea->MinLongitude  = 0;

      }
    }
}

static void ReadAltitude(TCHAR *Text_, AIRSPACE_ALT *Alt)
{
  TCHAR *Stop;
  TCHAR Text[128];
  TCHAR *pWClast = NULL;
  TCHAR *pToken;
  bool  fHasUnit=false;

  _tcsncpy(Text, Text_, sizeof(Text)/sizeof(Text[0]));
  Text[sizeof(Text)/sizeof(Text[0])-1] = '\0';

  _tcsupr(Text);

  pToken = strtok_r(Text, TEXT(" "), &pWClast);

  Alt->Altitude = 0;
  Alt->FL = 0;
  Alt->AGL = 0;
  Alt->Base = abUndef;

  while((pToken != NULL) && (*pToken != '\0')){

    if (isdigit(*pToken)) {
      double d = (double)StrToDouble(pToken, &Stop);
      if (Alt->Base == abFL){
        Alt->FL = d;
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      } else if (Alt->Base == abAGL) {
	Alt->AGL = d;
      } else {
        Alt->Altitude = d;
      }
      if (*Stop != '\0'){
        pToken = Stop;
        continue;
      }

    }

    else if (_tcscmp(pToken, TEXT("GND")) == 0) {
      // JMW support XXXGND as valid, equivalent to XXXAGL
      Alt->Base = abAGL;
      if (Alt->Altitude>0) {
	Alt->AGL = Alt->Altitude;
	Alt->Altitude = 0;
      } else {
	Alt->FL = 0;
	Alt->Altitude = 0;
	Alt->AGL = -1;
	fHasUnit = true;
      }
    }

    else if (_tcscmp(pToken, TEXT("SFC")) == 0) {
      Alt->Base = abAGL;
      Alt->FL = 0;
      Alt->Altitude = 0;
      Alt->AGL = -1;
      fHasUnit = true;
    }

    else if (_tcsstr(pToken, TEXT("FL")) == pToken){
      // this parses "FL=150" and "FL150"
      Alt->Base = abFL;
      fHasUnit = true;
      if (pToken[2] != '\0'){// no separator between FL and number
	pToken = &pToken[2];
	continue;
      }
    }

    else if ((_tcscmp(pToken, TEXT("FT")) == 0)
             || (_tcscmp(pToken, TEXT("F")) == 0)){
      Alt->Altitude = Alt->Altitude/TOFEET;
      fHasUnit = true;
    }

    else if (_tcscmp(pToken, TEXT("MSL")) == 0){
      Alt->Base = abMSL;
    }

    else if (_tcscmp(pToken, TEXT("M")) == 0){
      // JMW must scan for MSL before scanning for M
      fHasUnit = true;
    }

    else if (_tcscmp(pToken, TEXT("AGL")) == 0){
      Alt->Base = abAGL;
      Alt->AGL = Alt->Altitude;
      Alt->Altitude = 0;
    }

    else if (_tcscmp(pToken, TEXT("STD")) == 0){
      if (Alt->Base != abUndef) {
        // warning! multiple base tags
      }
      Alt->Base = abFL;
      Alt->FL = (Alt->Altitude * TOFEET) / 100;
      Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);

    }

    pToken = strtok_r(NULL, TEXT(" \t"), &pWClast);

  }

  if (!fHasUnit && (Alt->Base != abFL)) {
    // ToDo warning! no unit defined use feet or user alt unit
    // Alt->Altitude = Units::ToSysAltitude(Alt->Altitude);
    Alt->Altitude = Alt->Altitude/TOFEET;
    Alt->AGL = Alt->AGL/TOFEET;
  }

  if (Alt->Base == abUndef) {
    // ToDo warning! no base defined use MSL
    Alt->Base = abMSL;
  }

}

static void CalculateSector(TCHAR *Text)
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

    //	  if (bFillMode)	// Trig calcs not needed on first pass
    {
      FindLatitudeLongitude(CenterY, CenterX, StartBearing, Radius,
                            &TempPoint.Latitude,
                            &TempPoint.Longitude);
    }
    AddPoint(&TempPoint, &TempArea.NumPoints);

    StartBearing += Rotation *5 ;
  }

//  if (bFillMode)	// Trig calcs not needed on first pass
  {
    FindLatitudeLongitude(CenterY, CenterX, EndBearing, Radius,
                          &TempPoint.Latitude,
                          &TempPoint.Longitude);
  }
  AddPoint(&TempPoint, &TempArea.NumPoints);
}

static void CalculateArc(TCHAR *Text)
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

  DistanceBearing(CenterY, CenterX, StartLat, StartLon,
                  &Radius, &StartBearing);
  DistanceBearing(CenterY, CenterX, EndLat, EndLon,
                  NULL, &EndBearing);
  TempPoint.Latitude  = StartLat;
  TempPoint.Longitude = StartLon;
  AddPoint(&TempPoint, &TempArea.NumPoints);

  while(fabs(EndBearing-StartBearing) > 7.5)
  {
	  StartBearing += Rotation *5 ;

	  if(StartBearing > 360)
		  StartBearing -= 360;
	  if(StartBearing < 0)
		  StartBearing += 360;

	  if (bFillMode)	// Trig calcs not needed on first pass
	  {
            FindLatitudeLongitude(CenterY, CenterX, StartBearing, Radius,
                                  &TempPoint.Latitude,
                                  &TempPoint.Longitude);
	  }
    AddPoint(&TempPoint, &TempArea.NumPoints);
  }
  TempPoint.Latitude  = EndLat;
  TempPoint.Longitude = EndLon;
  AddPoint(&TempPoint, &TempArea.NumPoints);
}


static void ScanAirspaceCircleBounds(int i, double bearing) {
  double lat, lon;
  FindLatitudeLongitude(AirspaceCircle[i].Latitude,
                        AirspaceCircle[i].Longitude,
                        bearing, AirspaceCircle[i].Radius,
                        &lat, &lon);

  AirspaceCircle[i].bounds.minx = min(lon, AirspaceCircle[i].bounds.minx);
  AirspaceCircle[i].bounds.maxx = max(lon, AirspaceCircle[i].bounds.maxx);
  AirspaceCircle[i].bounds.miny = min(lat, AirspaceCircle[i].bounds.miny);
  AirspaceCircle[i].bounds.maxy = max(lat, AirspaceCircle[i].bounds.maxy);
}


static void FindAirspaceCircleBounds() {
  unsigned int i;
  for(i=0; i<NumberOfAirspaceCircles; i++) {
    AirspaceCircle[i].bounds.minx = AirspaceCircle[i].Longitude;
    AirspaceCircle[i].bounds.maxx = AirspaceCircle[i].Longitude;
    AirspaceCircle[i].bounds.miny = AirspaceCircle[i].Latitude;
    AirspaceCircle[i].bounds.maxy = AirspaceCircle[i].Latitude;
    ScanAirspaceCircleBounds(i,0);
    ScanAirspaceCircleBounds(i,90);
    ScanAirspaceCircleBounds(i,180);
    ScanAirspaceCircleBounds(i,270);
    AirspaceCircle[i].WarningLevel = 0; // clear warnings to initialise

    // JMW detect airspace that wraps across 180
    if ((AirspaceCircle[i].bounds.minx< -90) && (AirspaceCircle[i].bounds.maxx>90)) {
      double tmp = AirspaceCircle[i].bounds.minx;
      AirspaceCircle[i].bounds.minx = AirspaceCircle[i].bounds.maxx;
      AirspaceCircle[i].bounds.maxx = tmp;
    }
  }
}

static void FindAirspaceAreaBounds() {
  unsigned i, j;
  for(i=0; i<NumberOfAirspaceAreas; i++) {
    bool first = true;

    for(j= AirspaceArea[i].FirstPoint;
        j< AirspaceArea[i].FirstPoint+AirspaceArea[i].NumPoints; j++) {

      if (first) {

        CheckAirspacePoint(j);

        AirspaceArea[i].bounds.minx = AirspacePoint[j].Longitude;
        AirspaceArea[i].bounds.maxx = AirspacePoint[j].Longitude;
        AirspaceArea[i].bounds.miny = AirspacePoint[j].Latitude;
        AirspaceArea[i].bounds.maxy = AirspacePoint[j].Latitude;
        first = false;
      } else {
        AirspaceArea[i].bounds.minx = min(AirspacePoint[j].Longitude,
                                          AirspaceArea[i].bounds.minx);
        AirspaceArea[i].bounds.maxx = max(AirspacePoint[j].Longitude,
                                          AirspaceArea[i].bounds.maxx);
        AirspaceArea[i].bounds.miny = min(AirspacePoint[j].Latitude,
                                          AirspaceArea[i].bounds.miny);
        AirspaceArea[i].bounds.maxy = max(AirspacePoint[j].Latitude,
                                          AirspaceArea[i].bounds.maxy);
      }
    }

    // JMW detect airspace that wraps across 180
    if ((AirspaceArea[i].bounds.minx< -90) && (AirspaceArea[i].bounds.maxx>90)) {
      double tmp = AirspaceArea[i].bounds.minx;
      AirspaceArea[i].bounds.minx = AirspaceArea[i].bounds.maxx;
      AirspaceArea[i].bounds.maxx = tmp;
      for(j= AirspaceArea[i].FirstPoint;
          j< AirspaceArea[i].FirstPoint+AirspaceArea[i].NumPoints; j++) {
        if (AirspacePoint[i].Longitude<0) {
          AirspacePoint[i].Longitude += 360;
        }
      }
    }

    AirspaceArea[i].WarningLevel = 0; // clear warnings to initialise
  }
}


extern TCHAR szRegistryAirspaceFile[];
extern TCHAR szRegistryAdditionalAirspaceFile[];

// ToDo add exception handler to protect parser code against chrashes

void ReadAirspace(void)
{
  TCHAR	szFile1[MAX_PATH] = TEXT("\0");
  TCHAR	szFile2[MAX_PATH] = TEXT("\0");
  char zfilename[MAX_PATH];

  ZZIP_FILE *fp=NULL;
  ZZIP_FILE *fp2=NULL;

#if AIRSPACEUSEBINFILE > 0
  FILETIME LastWriteTime;
  FILETIME LastWriteTime2;
#endif

  GetRegistryString(szRegistryAirspaceFile, szFile1, MAX_PATH);
  ExpandLocalPath(szFile1);
  GetRegistryString(szRegistryAdditionalAirspaceFile, szFile2, MAX_PATH);
  ExpandLocalPath(szFile2);

  if (_tcslen(szFile1)>0) {
    unicode2ascii(szFile1, zfilename, MAX_PATH);
    fp  = zzip_fopen(zfilename, "rt");
  } else {
    /*
    static TCHAR  szMapFile[MAX_PATH] = TEXT("\0");
    GetRegistryString(szRegistryMapFile, szMapFile, MAX_PATH);
    ExpandLocalPath(szMapFile);
    wcscat(szMapFile,TEXT("/"));
    wcscat(szMapFile,TEXT("airspace.txt"));
    unicode2ascii(szMapFile, zfilename, MAX_PATH);
    fp  = zzip_fopen(zfilename, "rt");
    */
  }

  if (_tcslen(szFile2)>0) {
    unicode2ascii(szFile2, zfilename, MAX_PATH);
    fp2 = zzip_fopen(zfilename, "rt");
  }

  SetRegistryString(szRegistryAirspaceFile, TEXT("\0"));
  SetRegistryString(szRegistryAdditionalAirspaceFile, TEXT("\0"));

  if (fp != NULL){

    ReadAirspace(fp);
    zzip_fclose(fp);

    // file 1 was OK, so save it
    ContractLocalPath(szFile1);
    SetRegistryString(szRegistryAirspaceFile, szFile1);

    // also read any additional airspace
    if (fp2 != NULL) {
      ReadAirspace(fp2);
      zzip_fclose(fp2);

      // file 2 was OK, so save it
      ContractLocalPath(szFile2);
      SetRegistryString(szRegistryAdditionalAirspaceFile, szFile2);
    }
  }

  FindAirspaceAreaBounds();
  FindAirspaceCircleBounds();

}





double RangeAirspaceCircle(const double &longitude,
			   const double &latitude,
			   int i) {
  double distance;
  DistanceBearing(latitude,longitude,
                  AirspaceCircle[i].Latitude,
                  AirspaceCircle[i].Longitude,
                  &distance, NULL);
  return distance-AirspaceCircle[i].Radius;
}


bool CheckInsideLongitude(double longitude,
                         const double lon_min, const double lon_max) {
  if (lon_min<=lon_max) {
    // normal case
    return ((longitude>lon_min) && (longitude<lon_max));
  } else {
    // area goes across 180 degree boundary, so lon_min is +ve, lon_max is -ve (flipped)
    return ((longitude>lon_min) || (longitude<lon_max));
  }
}


bool InsideAirspaceCircle(const double &longitude,
			    const double &latitude,
			    int i) {

  if ((latitude> AirspaceCircle[i].bounds.miny) &&
      (latitude< AirspaceCircle[i].bounds.maxy) &&
      CheckInsideLongitude(longitude, AirspaceCircle[i].bounds.minx,
                           AirspaceCircle[i].bounds.maxx)) {

    if (RangeAirspaceCircle(longitude, latitude, i)<0) {
      return true;
    }
  }
  return false;
}


int FindAirspaceCircle(double Longitude,double Latitude, bool visibleonly)
{
  unsigned i;
 // int NearestIndex = 0;

  if(NumberOfAirspaceCircles == 0)
    {
      return -1;
    }

  for(i=0;i<NumberOfAirspaceCircles;i++) {
    if (MapWindow::iAirspaceMode[AirspaceCircle[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    if(AirspaceCircle[i].Visible || (!visibleonly)) {
      if(CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude,
			       AirspaceCircle[i].Top.Altitude)) {
	if (InsideAirspaceCircle(Longitude,Latitude,i)) {
	  return i;
	}
      }
    }
  }
  return -1;
}


BOOL CheckAirspaceAltitude(const double &Base, const double &Top)
{
  double alt;
  if (GPS_INFO.BaroAltitudeAvailable) {
    alt = GPS_INFO.BaroAltitude;
  } else {
    alt = GPS_INFO.Altitude;
  }

  switch (AltitudeMode)
    {
    case ALLON : return TRUE;

    case CLIP :
      if(Base < ClipAltitude)
	return TRUE;
      else
	return FALSE;

    case AUTO:
      if( ( alt > (Base - AltWarningMargin) )
	  && ( alt < (Top + AltWarningMargin) ))
	return TRUE;
      else
	return FALSE;

    case ALLBELOW:
      if(  (Base - AltWarningMargin) < alt )
	return  TRUE;
      else
	return FALSE;
    case INSIDE:
      if( ( alt >= (Base) ) && ( alt < (Top) ))
	return TRUE;
      else
        return FALSE;
    case ALLOFF : return FALSE;
    }
  return TRUE;
}


// hack, should be replaced with a data change notifier in the future...
void AirspaceQnhChangeNotify(double newQNH){

  int i;
  AIRSPACE_ALT *Alt;

  if (newQNH != lastQNH){

    for(i=0;i<(int)NumberOfAirspaceAreas;i++) {

      Alt = &AirspaceArea[i].Top;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }

      Alt = &AirspaceArea[i].Base;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }
    }

    for(i=0;i<(int)NumberOfAirspaceCircles;i++) {

      Alt = &AirspaceCircle[i].Top;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }

      Alt = &AirspaceCircle[i].Base;

      if (Alt->Base == abFL){
        Alt->Altitude = AltitudeToQNHAltitude((Alt->FL * 100)/TOFEET);
      }
    }

    lastQNH = newQNH;

  }


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
inline static double
isLeft( AIRSPACE_POINT P0, AIRSPACE_POINT P1, AIRSPACE_POINT P2 )
{
    return ( (P1.Longitude - P0.Longitude) * (P2.Latitude - P0.Latitude)
            - (P2.Longitude - P0.Longitude) * (P1.Latitude - P0.Latitude) );
}
//===================================================================

// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon V[n+1] with V[n]=V[0]
//      Return:  wn = the winding number (=0 only if P is outside V[])
static int
wn_PnPoly( AIRSPACE_POINT P, AIRSPACE_POINT* V, int n )
{
    int    wn = 0;    // the winding number counter

    // loop through all edges of the polygon
    for (int i=0; i<n; i++) {   // edge from V[i] to V[i+1]
        if (V[i].Latitude <= P.Latitude) {         // start y <= P.Latitude
            if (V[i+1].Latitude > P.Latitude)      // an upward crossing
                if (isLeft( V[i], V[i+1], P) > 0)  // P left of edge
                    ++wn;            // have a valid up intersect
        }
        else {                       // start y > P.Latitude (no test needed)
            if (V[i+1].Latitude <= P.Latitude)     // a downward crossing
                if (isLeft( V[i], V[i+1], P) < 0)  // P right of edge
                    --wn;            // have a valid down intersect
        }
    }
    return wn;
}
//===================================================================


bool InsideAirspaceArea(const double &longitude,
			  const double &latitude,
			  int i) {
  AIRSPACE_POINT thispoint;
  thispoint.Longitude = longitude;
  thispoint.Latitude = latitude;

  // first check if point is within bounding box
  if (
      (latitude> AirspaceArea[i].bounds.miny)&&
      (latitude< AirspaceArea[i].bounds.maxy)&&
      CheckInsideLongitude(longitude,
                           AirspaceArea[i].bounds.minx,
                           AirspaceArea[i].bounds.maxx)) {

    CheckAirspacePoint(AirspaceArea[i].FirstPoint);

    // it is within, so now do detailed polygon test
    if (wn_PnPoly(thispoint,
		  &AirspacePoint[AirspaceArea[i].FirstPoint],
		  AirspaceArea[i].NumPoints-1) != 0) {
      // we are inside the i'th airspace area
      return true;
    }
  }
  return false;
}


int FindAirspaceArea(double Longitude,double Latitude, bool visibleonly)
{
  unsigned i;

  if(NumberOfAirspaceAreas == 0)
    {
      return -1;
    }
  for(i=0;i<NumberOfAirspaceAreas;i++) {
    if (MapWindow::iAirspaceMode[AirspaceArea[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    if(AirspaceArea[i].Visible || (!visibleonly)) {
      if(CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude,
			       AirspaceArea[i].Top.Altitude)) {
	if (InsideAirspaceArea(Longitude,Latitude,i)) {
	  return i;
	}
      }
    }
  }
  // not inside any airspace
  return -1;
}





/////////////////////////////////////////////////////////////////////////////////


int FindNearestAirspaceCircle(double longitude, double latitude,
			      double *nearestdistance, double *nearestbearing,
			      double *height=NULL)
{
  unsigned int i;
//  int NearestIndex = 0;
  double Dist;
  int ifound = -1;

  if(NumberOfAirspaceCircles == 0) {
      return -1;
  }

  for(i=0;i<NumberOfAirspaceCircles;i++) {
    if (MapWindow::iAirspaceMode[AirspaceCircle[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }

    bool altok;
    if (height) {
      altok = ((*height>AirspaceCircle[i].Base.Altitude)&&
	       (*height<AirspaceCircle[i].Top.Altitude));
    } else {
      altok = CheckAirspaceAltitude(AirspaceCircle[i].Base.Altitude,
				    AirspaceCircle[i].Top.Altitude)==TRUE;
    }
    if(altok) {

      Dist = RangeAirspaceCircle(longitude, latitude, i);

      if(Dist < *nearestdistance ) {
	  *nearestdistance = Dist;
          DistanceBearing(latitude,
                          longitude,
                          AirspaceCircle[i].Latitude,
                          AirspaceCircle[i].Longitude,
                          NULL, nearestbearing);
	  if (Dist<0) {
	    // no need to continue search, inside
	    return i;
	  }
	  ifound = i;
      }
    }
  }
  return ifound;
}



// this is a slow function
// adapted from The Aviation Formulary 1.42

// finds the point along a distance dthis between p1 and p2, which are
// separated by dtotal
void IntermediatePoint(double lon1, double lat1,
		       double lon2, double lat2,
		       double dthis,
		       double dtotal,
		       double *lon3, double *lat3) {
  double A, B, x, y, z, d, f;
  /*
  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  */

  ASSERT(lat3 != NULL);
  ASSERT(lon3 != NULL);

  if ((lon1 == lon2) && (lat1 == lat2)){
    *lat3 = lat1;
    *lon3 = lon1;
    return;
  }

  if (dtotal>0) {
    f = dthis/dtotal;
    d = dtotal;
  } else {
    dtotal=1.0e-7;
    f = 0.0;
  }
  f = min(1.0,max(0.0,f));

  double coslat1 = cos(lat1);
  double coslat2 = cos(lat2);

  A=sin((1-f)*d)/sin(d);
  B=sin(f*d)/sin(d);
  x = A*coslat1*cos(lon1) +  B*coslat2*cos(lon2);
  y = A*coslat1*sin(lon1) +  B*coslat2*sin(lon2);
  z = A*sin(lat1)           +  B*sin(lat2);
  *lat3=atan2(z,sqrt(x*x+y*y))*RAD_TO_DEG;
  *lon3=atan2(y,x)*RAD_TO_DEG;
}

// finds cross track error in meters and closest point p4 between p3 and
// desired track p1-p2.
// very slow function!
double CrossTrackError(double lon1, double lat1,
                       double lon2, double lat2,
                       double lon3, double lat3,
                       double *lon4, double *lat4) {

  double dist_AD, crs_AD;
  DistanceBearing(lat1, lon1, lat3, lon3, &dist_AD, &crs_AD);
  dist_AD/= (RAD_TO_DEG * 111194.9267); crs_AD*= DEG_TO_RAD;

  double dist_AB, crs_AB;
  DistanceBearing(lat1, lon1, lat2, lon2, &dist_AB, &crs_AB);
  dist_AB/= (RAD_TO_DEG * 111194.9267); crs_AB*= DEG_TO_RAD;

  lat1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lat3 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  lon3 *= DEG_TO_RAD;

  double XTD; // cross track distance
  double ATD; // along track distance
  //  The "along track distance", ATD, the distance from A along the
  //  course towards B to the point abeam D

  double sindist_AD = sin(dist_AD);

  XTD = asin(sindist_AD*sin(crs_AD-crs_AB));

  double sinXTD = sin(XTD);
  ATD = asin(sqrt( sindist_AD*sindist_AD - sinXTD*sinXTD )/cos(XTD));

  if (lon4 && lat4) {
    IntermediatePoint(lon1, lat1, lon2, lat2, ATD, dist_AB,
		      lon4, lat4);
  }

  // units
  XTD *= (RAD_TO_DEG * 111194.9267);

  return XTD;
}


// this one uses screen coordinates to avoid as many trig functions
// as possible.. it means it is approximate but for our use it is ok.
double ScreenCrossTrackError(double lon1, double lat1,
		     double lon2, double lat2,
		     double lon3, double lat3,
		     double *lon4, double *lat4) {
  POINT p1, p2, p3;

  MapWindow::LatLon2Screen(lon1, lat1, p1);
  MapWindow::LatLon2Screen(lon2, lat2, p2);
  MapWindow::LatLon2Screen(lon3, lat3, p3);

  int v12x, v12y, v13x, v13y;

  v12x = p2.x-p1.x; v12y = p2.y-p1.y;
  v13x = p3.x-p1.x; v13y = p3.y-p1.y;

  int mag12 = isqrt4(v12x*v12x+v12y*v12y);
  if (mag12>1) {

    // projection of v13 along v12 = v12.v13/|v12|
    int proj = (v12x*v13x+v12y*v13y)/mag12;

    // distance between 3 and tangent to v12
//    int dist = abs(isqrt4(v13x*v13x+v13y*v13y-proj*proj));

    // fractional distance
    double f = min(1.0,max(0,proj*1.0/mag12));

    // location of 'closest' point
    int x, y;
    x = (int)((v12x)*f+p1.x);
    y = (int)((v12y)*f+p1.y);
    MapWindow::Screen2LatLon(x, y, *lon4, *lat4);
  } else {
    *lon4 = lon1;
    *lat4 = lat1;
  }

  // compute accurate distance
  double tmpd;
  DistanceBearing(lat3, lon3, *lat4, *lon4, &tmpd, NULL);
  return tmpd;
}


// Calculates projected distance from P3 along line P1-P2
double ProjectedDistance(double lon1, double lat1,
                         double lon2, double lat2,
                         double lon3, double lat3) {
  double lon4, lat4;

  CrossTrackError(lon1, lat1,
                  lon2, lat2,
                  lon3, lat3,
                   &lon4, &lat4);
  double tmpd;
  DistanceBearing(lat1, lon1, lat4, lon4, &tmpd, NULL);
  return tmpd;
}


double RangeAirspaceArea(const double &longitude,
			 const double &latitude,
			 int i, double *bearing) {

  // find nearest distance to line segment
  int j;
  double dist= 0;
  double nearestdistance = dist;
  double nearestbearing = *bearing;
  double lon4, lat4;
  for (j=0; j<(int)AirspaceArea[i].NumPoints-1; j++) {

    int p1 = AirspaceArea[i].FirstPoint+j;
    int p2 = AirspaceArea[i].FirstPoint+j+1;
    CheckAirspacePoint(p1);
    CheckAirspacePoint(p2);

    dist = ScreenCrossTrackError(
				 AirspacePoint[p1].Longitude,
				 AirspacePoint[p1].Latitude,
				 AirspacePoint[p2].Longitude,
				 AirspacePoint[p2].Latitude,
				 longitude, latitude,
				 &lon4, &lat4);
    if ((dist<nearestdistance)||(j==0)) {
      nearestdistance = dist;

      DistanceBearing(latitude, longitude,
                      lat4, lon4, NULL,
                      &nearestbearing);
    }
  }
  *bearing = nearestbearing;
  return nearestdistance;
}





int FindNearestAirspaceArea(double longitude,
			    double latitude,
			    double *nearestdistance,
			    double *nearestbearing,
			    double *height=NULL)
{
  unsigned i;
  int ifound = -1;
  bool inside=false;
  // location of point the target is abeam along line in airspace area

  if(NumberOfAirspaceAreas == 0)
    {
      return -1;
    }

  for(i=0;i<NumberOfAirspaceAreas;i++) {
    if (MapWindow::iAirspaceMode[AirspaceArea[i].Type]< 2) {
      // don't want warnings for this one
      continue;
    }
    bool altok;
    if (!height) {
      altok = CheckAirspaceAltitude(AirspaceArea[i].Base.Altitude,
				    AirspaceArea[i].Top.Altitude)==TRUE;
    } else {
      altok = ((*height<AirspaceArea[i].Top.Altitude)&&
	       (*height>AirspaceArea[i].Base.Altitude));
    }
    if(altok) {
      inside = InsideAirspaceArea(longitude, latitude, i);
      double dist, bearing;

      dist = RangeAirspaceArea(longitude, latitude, i, &bearing);

      if (dist< *nearestdistance) {
	*nearestdistance = dist;
	*nearestbearing = bearing;
	ifound = i;
      }
      if (inside) {
	// no need to continue the search
	*nearestdistance = -(*nearestdistance);
	return i;
      }
    }
  }
  // not inside any airspace, so return closest one
  return ifound;
}




////////////////////////
//
// Finds nearest airspace (whether circle or area) to the specified point.
// Returns -1 in foundcircle or foundarea if circle or area is not found
// Otherwise, returns index of the circle or area that is closest to the specified
// point.
//
// Also returns the distance and bearing to the boundary of the airspace,
// (and the vertical separation TODO).
//
// Distance <0 means interior.
//
// This only searches within a range of 100km of the target

void FindNearestAirspace(double longitude, double latitude,
			 double *nearestdistance, double *nearestbearing,
			 int *foundcircle, int *foundarea,
			 double *height)
{
  double nearestd1 = 100000; // 100km
  double nearestd2 = 100000; // 100km
  double nearestb1 = 0;
  double nearestb2 = 0;

  *foundcircle = FindNearestAirspaceCircle(longitude, latitude,
					   &nearestd1, &nearestb1, height);

  *foundarea = FindNearestAirspaceArea(longitude, latitude,
				       &nearestd2, &nearestb2, height);

  if ((*foundcircle>=0)&&(*foundarea<0)) {
      *nearestdistance = nearestd1;
      *nearestbearing = nearestb1;
      *foundarea = -1;
      return;
  }
  if ((*foundarea>=0)&&(*foundcircle<0)) {
      *nearestdistance = nearestd2;
      *nearestbearing = nearestb2;
      *foundcircle = -1;
      return;
  }


  if (nearestd1<nearestd2) {
    if (nearestd1<100000) {
      *nearestdistance = nearestd1;
      *nearestbearing = nearestb1;
      *foundarea = -1;
    }
  } else {
    if (nearestd2<100000) {
      *nearestdistance = nearestd2;
      *nearestbearing = nearestb2;
      *foundcircle = -1;
    }
  }
  return;
}


/////////////////////////////


static int _cdecl SortAirspaceAreaCompare(const void *elem1, const void *elem2 )
{
  if (AirspacePriority[((AIRSPACE_AREA *)elem1)->Type] >
      AirspacePriority[((AIRSPACE_AREA *)elem2)->Type])
    return (-1);
  if (AirspacePriority[((AIRSPACE_AREA *)elem1)->Type] <
      AirspacePriority[((AIRSPACE_AREA *)elem2)->Type])
    return (+1);

  // otherwise sort on height?
  return (0);
}

static int _cdecl SortAirspaceCircleCompare(const void *elem1, const void *elem2 )
{
  if (AirspacePriority[((AIRSPACE_CIRCLE *)elem1)->Type] >
      AirspacePriority[((AIRSPACE_CIRCLE *)elem2)->Type])
    return (-1);
  if (AirspacePriority[((AIRSPACE_CIRCLE *)elem1)->Type] <
      AirspacePriority[((AIRSPACE_CIRCLE *)elem2)->Type])
    return (+1);

  // otherwise sort on height?
  return (0);
}


void SortAirspace(void) {
  StartupStore(TEXT("SortAirspace\n"));

  // force acknowledgement before sorting
  ClearAirspaceWarnings(true, false);

  qsort(AirspaceArea,
	NumberOfAirspaceAreas,
	sizeof(AIRSPACE_AREA),
	SortAirspaceAreaCompare);

  qsort(AirspaceCircle,
	NumberOfAirspaceCircles,
	sizeof(AIRSPACE_CIRCLE),
	SortAirspaceCircleCompare);

}


/////////////


int line_line_intersection (const double x1, const double y1,
			    const double dx, const double dy,
			    const double x3, const double y3,
			    const double x4, const double y4,
			    double *u) {
  /*
  double a = sqr(dx)+sqr(dy);
  if (a <= 0) {
    return 0;
  }
  */
  double denom = (y4-y3)*dx-(x4-x3)*dy;
  if (denom == 0) {
    // lines are parallel
    return 0;
  }
  double ua = ((x4-x3)*(y1-y3)-(y4-y3)*(x1-x3))/denom;
  if ((ua<0) || (ua>1.0)) {
    // outside first line
    return 0;
  } else {
    double ub = (dx*(y1-y3)-dy*(x1-x3))/denom;
    if ((ub<0) || (ub>1.0)) {
      // outside second line
      return 0;
    } else {
      // inside both lines
      u[0] = ua;
      return 1;
    }
  }
}


bool line_rect_intersection (const double x1,
			     const double y1,
			     const double dx,
			     const double dy,
			     rectObj *bounds) {
  double u;

  // bottom line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->minx, bounds->miny,
			     bounds->maxx, bounds->miny,
			     &u)) return true;

  // left line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->minx, bounds->miny,
			     bounds->minx, bounds->maxy,
			     &u)) return true;

  // top line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->minx, bounds->maxy,
			     bounds->maxx, bounds->maxy,
			     &u)) return true;

  // right line
  if (line_line_intersection(x1, y1, dx, dy,
			     bounds->maxx, bounds->miny,
			     bounds->maxx, bounds->maxy,
			     &u)) return true;
  return false;
}


void ScanAirspaceLine(double *lats, double *lons, double *heights,
		      int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X])
{

  int i,j;
  unsigned int k;
  double latitude, longitude, Dist;
  double x1 = lons[0];
  double dx = lons[AIRSPACE_SCANSIZE_X-1]-x1;
  double y1 = lats[0];
  double dy = lats[AIRSPACE_SCANSIZE_X-1]-y1;
  double h_min = heights[0];
  double h_max = heights[AIRSPACE_SCANSIZE_H-1];

  rectObj lineRect;

  lineRect.minx = min(x1, x1+dx);
  lineRect.maxx = max(x1, x1+dx);
  lineRect.miny = min(y1, y1+dy);
  lineRect.maxy = max(y1, y1+dy);

  for(k=0;k<NumberOfAirspaceCircles;k++) {

    // ignore if outside scan height
    if (!((h_max<=AirspaceCircle[k].Base.Altitude)||
	  (h_min>=AirspaceCircle[k].Top.Altitude))) {

      // ignore if scan line doesn't intersect bounds
      if (msRectOverlap(&lineRect, &AirspaceCircle[k].bounds) &&
	  line_rect_intersection (x1, y1, dx, dy,
				  &AirspaceCircle[k].bounds)) {

	for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
	  latitude = lats[i];
	  longitude = lons[i];
	  if ((latitude> AirspaceCircle[k].bounds.miny)&&
	      (latitude< AirspaceCircle[k].bounds.maxy)&&
	      CheckInsideLongitude(longitude,
				   AirspaceCircle[k].bounds.minx,
				   AirspaceCircle[k].bounds.maxx)) {

	    DistanceBearing(latitude,longitude,
			    AirspaceCircle[k].Latitude,
			    AirspaceCircle[k].Longitude, &Dist, NULL);
	    Dist -= AirspaceCircle[k].Radius;

	    if(Dist < 0) {
	      for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
		if ((heights[j]>AirspaceCircle[k].Base.Altitude)&&
		    (heights[j]<AirspaceCircle[k].Top.Altitude)) {
		  airspacetype[j][i] = AirspaceCircle[k].Type;
		} // inside height
	      } // finished scanning height
	    } // inside
	  } // in bound
	} // finished scanning range
      } // line intersects
    } // within height
  } // finished scanning circles

  for(k=0;k<NumberOfAirspaceAreas;k++) {

    // ignore if outside scan height
    if (!((h_max<=AirspaceArea[k].Base.Altitude)||
	  (h_min>=AirspaceArea[k].Top.Altitude))) {

      // ignore if scan line doesn't intersect bounds
      if (msRectOverlap(&lineRect, &AirspaceArea[k].bounds) &&
	  line_rect_intersection (x1, y1, dx, dy,
				  &AirspaceArea[k].bounds)) {

	for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
	  latitude = lats[i];
	  longitude = lons[i];

	  if ((latitude> AirspaceArea[k].bounds.miny)&&
	      (latitude< AirspaceArea[k].bounds.maxy)&&
	      CheckInsideLongitude(longitude,
				   AirspaceArea[k].bounds.minx,
				   AirspaceArea[k].bounds.maxx)) {
	    AIRSPACE_POINT thispoint;
	    thispoint.Longitude = longitude;
	    thispoint.Latitude = latitude;

	    CheckAirspacePoint(AirspaceArea[k].FirstPoint);

	    if (wn_PnPoly(thispoint,
			  &AirspacePoint[AirspaceArea[k].FirstPoint],
			  AirspaceArea[k].NumPoints-1) != 0) {
	      for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
		if ((heights[j]>AirspaceArea[k].Base.Altitude)&&
		    (heights[j]<AirspaceArea[k].Top.Altitude)) {
		  airspacetype[j][i] = AirspaceArea[k].Type;
		} // inside height
	      } // finished scanning height
	    } // inside
	  } // in bound
	} // finished scanning range
      } // line intersects
    } // within height
  } // finished scanning areas
}


#if 0
// old...
void ScanAirspaceLine_old(double *lats, double *lons, double *heights,
		      int airspacetype[AIRSPACE_SCANSIZE_H][AIRSPACE_SCANSIZE_X])
{

  int i,j;
  unsigned int k;
  double latitude, longitude, height, Dist;

  for(k=0;k<NumberOfAirspaceCircles;k++) {
    for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
      latitude = lats[i];
      longitude = lons[i];
      if ((latitude> AirspaceCircle[k].bounds.miny)&&
	  (latitude< AirspaceCircle[k].bounds.maxy)&&
          CheckInsideLongitude(longitude,
                               AirspaceCircle[k].bounds.minx,
                               AirspaceCircle[k].bounds.maxx)) {

        DistanceBearing(latitude,longitude,
                        AirspaceCircle[k].Latitude,
                        AirspaceCircle[k].Longitude, &Dist, NULL);
	Dist -= AirspaceCircle[k].Radius;

	if(Dist < 0) {
	  for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
	    height = heights[j];
	    if ((height>AirspaceCircle[k].Base.Altitude)&&
		(height<AirspaceCircle[k].Top.Altitude)) {
	      airspacetype[j][i] = AirspaceCircle[k].Type;
	    } // inside height
	  } // finished scanning height
	} // inside
      } // in bound
    } // finished scanning range
  } // finished scanning circles

  for(k=0;k<NumberOfAirspaceAreas;k++) {
    for (i=0; i<AIRSPACE_SCANSIZE_X; i++) {
      latitude = lats[i];
      longitude = lons[i];

      if ((latitude> AirspaceArea[k].bounds.miny)&&
	  (latitude< AirspaceArea[k].bounds.maxy)&&
          CheckInsideLongitude(longitude,
                               AirspaceArea[k].bounds.minx,
                               AirspaceArea[k].bounds.maxx)) {
	AIRSPACE_POINT thispoint;
	thispoint.Longitude = longitude;
	thispoint.Latitude = latitude;

        CheckAirspacePoint(AirspaceArea[k].FirstPoint);

	if (wn_PnPoly(thispoint,
		      &AirspacePoint[AirspaceArea[k].FirstPoint],
		      AirspaceArea[k].NumPoints-1) != 0) {
	  for (j=0; j<AIRSPACE_SCANSIZE_H; j++) {
	    height = heights[j];
	    if ((height>AirspaceArea[k].Base.Altitude)&&
		(height<AirspaceArea[k].Top.Altitude)) {
	      airspacetype[j][i] = AirspaceArea[k].Type;
	    } // inside height
	  } // finished scanning height
	} // inside
      } // in bound
    } // finished scanning range
  } // finished scanning areas

}
#endif


#ifdef DEBUG
void DumpAirspaceFile(void){

  FILE * fp;
  int i;

  fp  = _tfopen(TEXT("XCSoarAirspace.dmp"), TEXT("wt"));

  for (i=0; i < (int)NumberOfAirspaceAreas; i++){

    _ftprintf(fp, TEXT("*** Aera id: %d %s "), i, AirspaceArea[i].Name);

    switch (AirspaceArea[i].Type){
      case RESTRICT:
        _ftprintf(fp, TEXT("Restricted")); break;
      case PROHIBITED:
        _ftprintf(fp, TEXT("Prohibited")); break;
      case DANGER:
        _ftprintf(fp, TEXT("Danger Area")); break;
      case CLASSA:
        _ftprintf(fp, TEXT("Class A")); break;
      case CLASSB:
        _ftprintf(fp, TEXT("Class B")); break;
      case CLASSC:
        _ftprintf(fp, TEXT("Class C")); break;
      case CLASSD:
        _ftprintf(fp, TEXT("Class D")); break;
      case CLASSE:
        _ftprintf(fp, TEXT("Class E")); break;
      case CLASSF:
        _ftprintf(fp, TEXT("Class F")); break;
      case NOGLIDER:
        _ftprintf(fp, TEXT("No Glider")); break;
      case CTR:
        _ftprintf(fp, TEXT("CTR")); break;
      case WAVE:
        _ftprintf(fp, TEXT("Wave")); break;
      default:
        _ftprintf(fp, TEXT("Unknown"));
    }

    _ftprintf(fp, TEXT(")\r\n"), i);

    switch (AirspaceArea[i].Top.Base){
      case abUndef:
        _ftprintf(fp, TEXT("  Top  : %.0f[m] %.0f[ft] [?]\r\n"), AirspaceArea[i].Top.Altitude, AirspaceArea[i].Top.Altitude*TOFEET);
      break;
      case abMSL:
        _ftprintf(fp, TEXT("  Top  : %.0f[m] %.0f[ft] [MSL]\r\n"), AirspaceArea[i].Top.Altitude, AirspaceArea[i].Top.Altitude*TOFEET);
      break;
      case abAGL:
        _ftprintf(fp, TEXT("  Top  : %.0f[m] %.0f[ft] [AGL]\r\n"), AirspaceArea[i].Top.AGL, AirspaceArea[i].Top.AGL*TOFEET);
      break;
      case abFL:
        _ftprintf(fp, TEXT("  Top  : FL %.0f (%.0f[m] %.0f[ft])\r\n"), AirspaceArea[i].Top.FL, AirspaceArea[i].Top.Altitude, AirspaceArea[i].Top.Altitude*TOFEET);
      break;
    }

    switch (AirspaceArea[i].Base.Base){
      case abUndef:
        _ftprintf(fp, TEXT("  Base : %.0f[m] %.0f[ft] [?]\r\n"), AirspaceArea[i].Base.Altitude, AirspaceArea[i].Base.Altitude*TOFEET);
      break;
      case abMSL:
        _ftprintf(fp, TEXT("  Base : %.0f[m] %.0f[ft] [MSL]\r\n"), AirspaceArea[i].Base.Altitude, AirspaceArea[i].Base.Altitude*TOFEET);
      break;
      case abAGL:
        _ftprintf(fp, TEXT("  Base : %.0f[m] %.0f[ft] [AGL]\r\n"), AirspaceArea[i].Base.AGL, AirspaceArea[i].Base.AGL*TOFEET);
      break;
      case abFL:
        _ftprintf(fp, TEXT("  Base : FL %.0f (%.0f[m] %.0f[ft])\r\n"), AirspaceArea[i].Base.FL, AirspaceArea[i].Base.Altitude, AirspaceArea[i].Base.Altitude*TOFEET);
      break;
    }

    _ftprintf(fp, TEXT("\r\n"), i);
  }

  for (i=0; i < (int)NumberOfAirspaceCircles; i++){

    _ftprintf(fp, TEXT("\r\n*** Circle id: %d %s ("), i, AirspaceCircle[i].Name);

    switch (AirspaceArea[i].Type){
      case RESTRICT:
        _ftprintf(fp, TEXT("Restricted")); break;
      case PROHIBITED:
        _ftprintf(fp, TEXT("Prohibited")); break;
      case DANGER:
        _ftprintf(fp, TEXT("Danger Area")); break;
      case CLASSA:
        _ftprintf(fp, TEXT("Class A")); break;
      case CLASSB:
        _ftprintf(fp, TEXT("Class B")); break;
      case CLASSC:
        _ftprintf(fp, TEXT("Class C")); break;
      case CLASSD:
        _ftprintf(fp, TEXT("Class D")); break;
      case CLASSE:
        _ftprintf(fp, TEXT("Class E")); break;
      case CLASSF:
        _ftprintf(fp, TEXT("Class F")); break;
      case NOGLIDER:
        _ftprintf(fp, TEXT("No Glider")); break;
      case CTR:
        _ftprintf(fp, TEXT("CTR")); break;
      case WAVE:
        _ftprintf(fp, TEXT("Wave")); break;
      default:
        _ftprintf(fp, TEXT("Unknown"));
    }

    _ftprintf(fp, TEXT(")\r\n"), i);

    switch (AirspaceCircle[i].Top.Base){
      case abUndef:
        _ftprintf(fp, TEXT("  Top  : %.0f[m] %.0f[ft] [?]\r\n"), AirspaceCircle[i].Top.Altitude, AirspaceCircle[i].Top.Altitude*TOFEET);
      break;
      case abMSL:
        _ftprintf(fp, TEXT("  Top  : %.0f[m] %.0f[ft] [MSL]\r\n"), AirspaceCircle[i].Top.Altitude, AirspaceCircle[i].Top.Altitude*TOFEET);
      break;
      case abAGL:
        _ftprintf(fp, TEXT("  Top  : %.0f[m] %.0f[ft] [AGL]\r\n"), AirspaceCircle[i].Top.AGL, AirspaceCircle[i].Top.AGL*TOFEET);
      break;
      case abFL:
        _ftprintf(fp, TEXT("  Top  : FL %.0f (%.0f[m] %.0f[ft])\r\n"), AirspaceCircle[i].Top.FL, AirspaceCircle[i].Top.Altitude, AirspaceCircle[i].Top.Altitude*TOFEET);
      break;
    }

    switch (AirspaceCircle[i].Base.Base){
      case abUndef:
        _ftprintf(fp, TEXT("  Base : %.0f[m] %.0f[ft] [?]\r\n"), AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Base.Altitude*TOFEET);
      break;
      case abMSL:
        _ftprintf(fp, TEXT("  Base : %.0f[m] %.0f[ft] [MSL]\r\n"), AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Base.Altitude*TOFEET);
      break;
      case abAGL:
        _ftprintf(fp, TEXT("  Base : %.0f[m] %.0f[ft] [AGL]\r\n"), AirspaceCircle[i].Base.AGL, AirspaceCircle[i].Base.AGL*TOFEET);
      break;
      case abFL:
        _ftprintf(fp, TEXT("  Base : FL %.0f (%.0f[m] %.0f[ft])\r\n"), AirspaceCircle[i].Base.FL, AirspaceCircle[i].Base.Altitude, AirspaceCircle[i].Base.Altitude*TOFEET);
      break;
    }

  _ftprintf(fp, TEXT("\r\n"), i);

  }

  fclose(fp);

}
#endif

///////////////////////////////////////////////////////////////////////////////
