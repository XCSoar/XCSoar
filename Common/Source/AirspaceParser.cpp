/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

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

#include "Airspace.h"
#include "Dialogs.h"
#include "Language.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "RasterTerrain.h"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "Registry.hpp"
#include "Math/Pressure.h"
#include "LocalPath.hpp"
#include "Components.hpp"
#include "Calculations.h" // TODO danger! ClearAirspaceWarnings

#include <windows.h>
#include <commctrl.h>
#include <math.h>

#include "wcecompat/ts_string.h"
#include "Compatibility/string.h"

#include <assert.h>

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
static void AddAirspaceCircle(AIRSPACE_AREA *Temp, const double aCenterX, const double aCenterY, const double Radius);
static void AddPoint(AIRSPACE_POINT *Temp, unsigned *AreaPointCount);
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
static int LineCount;

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
					_T("CTR"),
					_T("A"),
					_T("B"),
					_T("C"),
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
					CTR,
					CLASSA,
					CLASSB,
					CLASSC,
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
  DeleteAirspace();
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

  airspace_QNH = QNH;

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

    if (MessageBoxX(gettext(TEXT("Internal Airspace Parser Error!\r\nPlease send this Airspacefile to Support")),
                    gettext(TEXT("Airspace")), MB_OKCANCEL) == IDCANCEL) {
    }

  }

#ifndef NDEBUG
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
          double Radius = StrToDouble(&TempString[2],NULL);
          Radius = (Radius * NAUTICALMILESTOMETRES);
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
    _stprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
              gettext(TEXT("Parse Error at Line")),
              LineCount, TempString,
              gettext(TEXT("Line skipped.")));
    if (MessageBoxX(sTmp, gettext(TEXT("Airspace")),
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
              _stprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
                        gettext(TEXT("Parse Error at Line")),
                        LineCount, TempString,
                        gettext(TEXT("Line skipped.")));
              if (MessageBoxX(sTmp,
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
              _stprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
                        gettext(TEXT("Parse Error at Line")),
                        LineCount, TempString,
                        gettext(TEXT("Line skipped.")));
              if (MessageBoxX(sTmp,
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
          _stprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
                    gettext(TEXT("Parse Error at Line")),
                    LineCount, TempString,
                    gettext(TEXT("Line skipped.")));
          if (MessageBoxX(sTmp, gettext(TEXT("Airspace")),
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
  const TCHAR *Stop;

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

    terrain.Lock();
    // want most accurate rounding here
    terrain.SetTerrainRounding(0,0);

    double th =
      terrain.GetTerrainHeight(av_lat, av_lon);

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
    // JMW TODO enhancement: complain if out of terrain range (th<0)
    terrain.Unlock();
  }
}


static void AddAirspaceCircle(AIRSPACE_AREA *Temp,
                              const double aCenterX,
			      const double aCenterY,
			      const double aRadius)
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
      NewCircle->Latitude = aCenterY;
      NewCircle->Longitude = aCenterX;
      NewCircle->Radius = aRadius;
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

  assert(Temp != NULL);
  assert(AreaPointCount != NULL);

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
  const TCHAR *Stop;
  TCHAR Text[128];
  TCHAR *pWClast = NULL;
  const TCHAR *pToken;
  bool  fHasUnit=false;

  _tcsncpy(Text, Text_, sizeof(Text)/sizeof(Text[0]));
  Text[sizeof(Text)/sizeof(Text[0])-1] = '\0';

  _tcsupr(Text);

  pToken = strtok_r(Text, (TCHAR*)TEXT(" "), &pWClast);

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

    else if (_tcscmp(pToken, TEXT("UNL")) == 0) {
      // JMW added Unlimited (used by WGC2008)
      Alt->Base = abMSL;
      Alt->AGL = -1;
      Alt->Altitude = 50000;
    }

    pToken = strtok_r(NULL, (TCHAR*)TEXT(" \t"), &pWClast);

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
  const TCHAR *Stop;

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
    } else {
      StartupStore(TEXT("No airspace file 2\n"));
    }
  } else {
    StartupStore(TEXT("No airspace file 1\n"));
  }

  FindAirspaceAreaBounds();
  FindAirspaceCircleBounds();

}


#ifndef NDEBUG
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

