/*

Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "AirspaceDatabase.hpp"
#include "AirspaceWarning.h"
#include "Interface.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "Math/Pressure.h"
#include "options.h"
#include "Compatibility/string.h"

#include <math.h>
#include <tchar.h>
#include <ctype.h>
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

static void
DumpAirspaceFile(const AirspaceDatabase &airspace_database);

static bool StartsWith(TCHAR *Text, const TCHAR *LookFor);
static bool ReadCoords(TCHAR *Text, double *X, double *Y);

static void
AddAirspaceCircle(AirspaceDatabase &airspace_database, AIRSPACE_AREA *Temp,
                  const double aCenterX, const double aCenterY,
                  const double Radius, unsigned &NumberOfAirspaceCircles);

static void
AddPoint(AirspaceDatabase &airspace_database, AIRSPACE_POINT *Temp,
         unsigned *AreaPointCount, unsigned &NumberOfAirspacePoints);

static void
AddArea(AirspaceDatabase &airspace_database, AIRSPACE_AREA *Temp,
        unsigned &NumberOfAirspaceAreas);

static void ReadAltitude(TCHAR *Text, AIRSPACE_ALT *Alt);

static void
CalculateArc(AirspaceDatabase &airspace_database, TCHAR *Text,
             unsigned &NumberOfAirspacePoints);

static void
CalculateSector(AirspaceDatabase &airspace_database, TCHAR *Text,
                unsigned &NumberOfAirspacePoints);

static bool
ParseLine(AirspaceDatabase &airspace_database, int nLineType,
          unsigned &NumberOfAirspacePoints, unsigned &NumberOfAirspaceAreas,
          unsigned &NumberOfAirspaceCircles);

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

void CloseAirspace(AirspaceDatabase &airspace_database) {
  AirspaceWarnListClear(airspace_database);
  DeleteAirspace(airspace_database);
}

// this can now be called multiple times to load several airspaces.
// to start afresh, call CloseAirspace()

static void
ReadAirspace(AirspaceDatabase &airspace_database, ZZIP_FILE *fp)
{
  StartupStore(TEXT("ReadAirspace\n"));
  int	Tock = 0;
  DWORD	dwStep;
  DWORD	dwPos;
  DWORD	dwOldPos = 0L;
  int	nLineType;

  unsigned NumberOfAirspacePointsPass[2] = { 0, 0 };
  unsigned NumberOfAirspaceAreasPass[2] = { 0, 0 };
  unsigned NumberOfAirspaceCirclesPass[2] = { 0, 0 };

  LineCount = 0;

  airspace_database.SetQNH(QNH);

  XCSoarInterface::CreateProgressDialog(gettext(TEXT("Loading Airspace File...")));
  // Need step size finer than default 10
  XCSoarInterface::SetProgressStepSize(5);
  dwStep = zzip_file_size(fp) / 10L;

  TempArea.FirstPoint = airspace_database.NumberOfAirspacePoints;

  bFillMode = false;
  bWaiting = true;
  XCSoarInterface::StepProgressDialog();
  while((nLineType = GetNextLine(fp, TempString)) >= 0) {
    Tock++;
    Tock %= 50;
    if (Tock == 0) {
      dwPos = zzip_tell(fp);
      if ((dwPos - dwOldPos) >= dwStep) {
        XCSoarInterface::StepProgressDialog();
        dwOldPos = dwPos;
      }
    }

    if (!ParseLine(airspace_database, nLineType, NumberOfAirspacePointsPass[0],
                   NumberOfAirspaceAreasPass[0],
                   NumberOfAirspaceCirclesPass[0])) {
      CloseAirspace(airspace_database);
      return;
    }
  }

  // Process final area (if any). bFillMode is false.  JG 10-Nov-2005
  if (!bWaiting)
    NumberOfAirspaceAreasPass[0]++; // ????

  // allocate new memory
  size_t screen_point_size = (airspace_database.NumberOfAirspacePoints +
                              NumberOfAirspacePointsPass[0]) *
    sizeof(AirspaceScreenPoint[0]);
  POINT *new_points = AirspaceScreenPoint == NULL
    ? (POINT *)LocalAlloc(LMEM_FIXED, screen_point_size)
    : (POINT *)LocalReAlloc(AirspaceScreenPoint, LMEM_FIXED,
                            screen_point_size);
  if (new_points != NULL)
    AirspaceScreenPoint = new_points;

  if ((screen_point_size > 0 && new_points == NULL) ||
      !airspace_database.GrowPoints(NumberOfAirspacePointsPass[0]) ||
      !airspace_database.GrowAreas(NumberOfAirspaceAreasPass[0]) ||
      !airspace_database.GrowCircles(NumberOfAirspaceCirclesPass[0])) {
    // can't allocate memory, so delete everything
    airspace_database.Clear();

    if (AirspaceScreenPoint != NULL) {
      LocalFree((HLOCAL)AirspaceScreenPoint);
      AirspaceScreenPoint = NULL;
    }

    return;
  }

  // ok, start the read
  TempArea.FirstPoint = airspace_database.NumberOfAirspacePoints;
  zzip_seek(fp, 0, SEEK_SET );
  LineCount = -1;

  bFillMode = true;
  bWaiting = true;
  dwOldPos = 0L;
  XCSoarInterface::StepProgressDialog();
  CenterY = CenterX = 0;
  Rotation = 1;

  while((nLineType = GetNextLine(fp, TempString)) >= 0) {
    Tock++;
    Tock %= 50;

    if (Tock == 0) {
      dwPos = zzip_tell(fp);
      if ((dwPos - dwOldPos) >= dwStep) {
        XCSoarInterface::StepProgressDialog();
        dwOldPos = dwPos;
      }
    }

    ParseLine(airspace_database, nLineType, NumberOfAirspacePointsPass[1],
              NumberOfAirspaceAreasPass[1],
              NumberOfAirspaceCirclesPass[1]);
  }

  // Process final area (if any). bFillMode is true.  JG 10-Nov-2005
  if (!bWaiting)
    AddArea(airspace_database, &TempArea, NumberOfAirspaceAreasPass[1]);

  if (NumberOfAirspacePointsPass[0] != NumberOfAirspacePointsPass[1]
      || NumberOfAirspaceAreasPass[0] != NumberOfAirspaceAreasPass[1]
      || NumberOfAirspaceCirclesPass[0] != NumberOfAirspaceCirclesPass[1]){

    if (MessageBoxX(gettext(TEXT("Internal Airspace Parser Error!\r\nPlease send this Airspacefile to Support")),
                    gettext(TEXT("Airspace")), MB_OKCANCEL) == IDCANCEL) {
    }

  }

#ifndef NDEBUG
  // only do this if debugging
  DumpAirspaceFile(airspace_database);
#endif

//  if(AirspacePoint != NULL)  LocalFree((HLOCAL)AirspacePoint);

}

static bool
ParseLine(AirspaceDatabase &airspace_database, int nLineType,
          unsigned &NumberOfAirspacePoints, unsigned &NumberOfAirspaceAreas,
          unsigned &NumberOfAirspaceCircles)
{
  int nIndex;

  switch (nLineType) {
  case k_nLtAC:
    if (bFillMode) {
      if (!bWaiting)
        AddArea(airspace_database, &TempArea, NumberOfAirspaceAreas);
      TempArea.NumPoints = 0;
      TempArea.Type = OTHER;
      for (nIndex = 0; nIndex < k_nAreaCount; nIndex++) {
        if (StartsWith(&TempString[3], k_strAreaStart[nIndex])) {
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
    if (bFillMode) {
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
    if (StartsWith(&TempString[2], _T("X=")) ||
        StartsWith(&TempString[2], _T("x="))) {
      if (ReadCoords(&TempString[4],&CenterX, &CenterY))
        break;
    } else if (StartsWith(&TempString[2], _T("D=-")) ||
               StartsWith(&TempString[2], _T("d=-"))) {
      Rotation = -1;
      break;
    } else if (StartsWith(&TempString[2], _T("D=+")) ||
             StartsWith(&TempString[2], _T("d=+"))) {
      Rotation = +1;
      break;
    } else if (StartsWith(&TempString[2], _T("Z")) ||
               StartsWith(&TempString[2], _T("z"))) {
      // ToDo Display Zool Level
      break;
    } else if (StartsWith(&TempString[2], _T("W")) ||
               StartsWith(&TempString[2], _T("w"))) {
      // ToDo width of an airway
      break;
    } else if (StartsWith(&TempString[2], _T("T")) ||
               StartsWith(&TempString[2], _T("t"))) {
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
    AddPoint(airspace_database, &TempPoint, &TempArea.NumPoints,
             NumberOfAirspacePoints);
    // TempArea.NumPoints++;
    break;

  case k_nLtDB:
    CalculateArc(airspace_database, TempString, NumberOfAirspacePoints);
    break;

  case k_nLtDA:
    CalculateSector(airspace_database, TempString, NumberOfAirspacePoints);
    break;

  case k_nLtDC:
    if (bFillMode) {
      double Radius = _tcstod(&TempString[2], NULL);
      Radius = (Radius * NAUTICALMILESTOMETRES);
      AddAirspaceCircle(airspace_database, &TempArea, CenterX, CenterY, Radius,
                        NumberOfAirspaceCircles);
    } else
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
  TCHAR *Comment;
  int nSize;
  int nLineType = -1;
  TCHAR sTmp[READLINE_LENGTH];

  while (ReadString(fp, READLINE_LENGTH, Text)){
    // JMW was ReadStringX

    LineCount++;

    nSize = _tcslen(Text);

    // Ignore lines less than 3 characters
    // or starting with comment char
    if((nSize < 3) || (Text[0] == _T('*')))
      continue;

    // build a upercase copy of the tags
    _tcsncpy(sTmp, Text, sizeof(sTmp)/sizeof(sTmp[0]));
    sTmp[sizeof(sTmp)/sizeof(sTmp[0])-1] = '\0';
    _tcsupr(sTmp);

    // Only return expected lines
    switch (sTmp[0]) {
    case _T('A'):
      switch (sTmp[1]) {
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
          _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
                    gettext(_T("Parse Error at Line")),
                    LineCount, TempString,
                    gettext(_T("Line skipped.")));
          if (MessageBoxX(sTmp,
                          gettext(_T("Airspace")),
                          MB_OKCANCEL) == IDCANCEL)
            return(-1);
        }
        continue;
      }

      break;

    case _T('D'):
      switch (sTmp[1]) {
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
        if (bFillMode) {
          _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
                    gettext(_T("Parse Error at Line")),
                    LineCount, TempString,
                    gettext(_T("Line skipped.")));
          if (MessageBoxX(sTmp,
                          gettext(_T("Airspace")),
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
      if (bFillMode) {
        _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
                  gettext(_T("Parse Error at Line")),
                  LineCount, TempString,
                  gettext(_T("Line skipped.")));
        if (MessageBoxX(sTmp, gettext(_T("Airspace")),
                        MB_OKCANCEL) == IDCANCEL)
          return(-1);
      }
      continue;
    }

    if (nLineType >= 0) {// Valid line found
      // Strip comments and newline chars from end of line
      Comment = _tcschr(Text, _T('*'));
      if (Comment != NULL) {
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

  Ydeg = (double)_tcstod(Text, &Stop);
  if ((Text == Stop) || (*Stop =='\0')) goto OnError;
  Stop++;
  Ymin = (double)_tcstod(Stop, &Stop);
  if (Ymin<0 || Ymin >=60){
    // ToDo
  }
  if (*Stop =='\0') goto OnError;
  if(*Stop == ':'){
    Stop++;
    if (*Stop =='\0')
      goto OnError;
    Ysec = (double)_tcstod(Stop, &Stop);
    if (Ysec<0 || Ysec >=60) {
      // ToDo
    }
  }

  *Y = Ysec/3600 + Ymin/60 + Ydeg;

  if (*Stop == ' ')
    Stop++;

  if (*Stop =='\0') goto OnError;
  if ((*Stop == 'S') || (*Stop == 's'))
      *Y = *Y * -1;

  Stop++;
  if (*Stop =='\0') goto OnError;

  Xdeg = (double)_tcstod(Stop, &Stop);
  Stop++;
  Xmin = (double)_tcstod(Stop, &Stop);
  if(*Stop == ':'){
    Stop++;
    if (*Stop =='\0')
      goto OnError;
    Xsec = (double)_tcstod(Stop, &Stop);
  }

  *X = Xsec/3600 + Xmin/60 + Xdeg;

  if (*Stop == ' ')
    Stop++;
  if (*Stop =='\0') goto OnError;
  if((*Stop == 'W') || (*Stop == 'w'))
    *X = *X * -1;

  if (*X<-180)
    *X+= 360;

  if (*X>180)
    *X-= 360;

  return(true);

OnError:
  return(false);

}

static void
AddAirspaceCircle(AirspaceDatabase &airspace_database, AIRSPACE_AREA *Temp,
                  const double aCenterX,
                  const double aCenterY,
                  const double aRadius,
                  unsigned &NumberOfAirspaceCircles)
{
  AIRSPACE_CIRCLE *NewCircle = NULL;

  ++NumberOfAirspaceCircles;
  if (!bFillMode)
    return;

  NewCircle = airspace_database.AppendCircle();

  _tcscpy(NewCircle->Name , Temp->Name);
  NewCircle->Location.Latitude = aCenterY;
  NewCircle->Location.Longitude = aCenterX;
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
}

static void
AddPoint(AirspaceDatabase &airspace_database, AIRSPACE_POINT *Temp,
         unsigned *AreaPointCount, unsigned &NumberOfAirspacePoints)
{
  AIRSPACE_POINT *NewPoint = NULL;

  /*
  if(!bFillMode)
    {
      NumberOfAirspacePoints++;
    }
  else
    {
      NewPoint = &AirspacePoint[NumberOfAirspacePoints];
      NumberOfAirspacePoints++;

      NewPoint->Latitude  = Temp->Latitude;
      NewPoint->Longitude = Temp->Longitude;
    }
  */

  ++NumberOfAirspacePoints;
  if (!bFillMode)
    return;

  assert(Temp != NULL);
  assert(AreaPointCount != NULL);

  NewPoint = airspace_database.AppendPoint();
  NewPoint->Latitude = Temp->Latitude;
  NewPoint->Longitude = Temp->Longitude;

  (*AreaPointCount)++;
}

static void
AddArea(AirspaceDatabase &airspace_database, AIRSPACE_AREA *Temp,
        unsigned &NumberOfAirspaceAreas)
{
  AIRSPACE_AREA *NewArea = NULL;

  ++NumberOfAirspaceAreas;
  if (!bFillMode)
    return;

  NewArea = airspace_database.AppendArea();

  _tcscpy(NewArea->Name , Temp->Name);
  NewArea->Type = Temp->Type;
  NewArea->Base.Altitude = Temp->Base.Altitude;
  NewArea->Base.FL = Temp->Base.FL ;
  NewArea->Base.Base = Temp->Base.Base;
  NewArea->Base.AGL = Temp->Base.AGL ;
  NewArea->NumPoints = Temp->NumPoints;
  NewArea->Top.Altitude = Temp->Top.Altitude;
  NewArea->Top.FL = Temp->Top.FL;
  NewArea->Top.Base = Temp->Top.Base;
  NewArea->Top.AGL = Temp->Top.AGL;
  NewArea->FirstPoint = Temp->FirstPoint;
  NewArea->Ack.AcknowledgedToday = false;
  NewArea->Ack.AcknowledgementTime = 0;
  NewArea->_NewWarnAckNoBrush = false;

  Temp->FirstPoint = Temp->FirstPoint + Temp->NumPoints;

  if (Temp->NumPoints > 0) {
    const AIRSPACE_POINT *PointList =
      &airspace_database.AirspacePoint[NewArea->FirstPoint];
    NewArea->maxBound.Latitude = -90;
    NewArea->minBound.Latitude = 90;
    NewArea->maxBound.Longitude = -180;
    NewArea->minBound.Longitude = 180;

    for (unsigned i = 0; i < Temp->NumPoints; i++) {
      if (PointList[i].Latitude > NewArea->maxBound.Latitude)
        NewArea->maxBound.Latitude = PointList[i].Latitude;
      if (PointList[i].Latitude < NewArea->minBound.Latitude)
        NewArea->minBound.Latitude = PointList[i].Latitude;

      if (PointList[i].Longitude  > NewArea->maxBound.Longitude)
        NewArea->maxBound.Longitude = PointList[i].Longitude;
      if (PointList[i].Longitude  < NewArea->minBound.Longitude)
        NewArea->minBound.Longitude = PointList[i].Longitude;
    }
  } else {
    NewArea->maxBound.Latitude = 0;
    NewArea->minBound.Latitude = 0;
    NewArea->maxBound.Longitude = 0;
    NewArea->minBound.Longitude = 0;
  }
}

static void ReadAltitude(TCHAR *Text_, AIRSPACE_ALT *Alt)
{
  TCHAR *Stop;
  TCHAR Text[128];
  TCHAR *pWClast = NULL;
  const TCHAR *pToken;
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
      double d = (double)_tcstod(pToken, &Stop);
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

static void
CalculateSector(AirspaceDatabase &airspace_database, TCHAR *Text,
                unsigned &NumberOfAirspacePoints)
{
  double Radius;
  double StartBearing;
  double EndBearing;
  TCHAR *Stop;

  Radius = NAUTICALMILESTOMETRES * (double)_tcstod(&Text[2], &Stop);
  StartBearing = (double)_tcstod(&Stop[1], &Stop);
  EndBearing = (double)_tcstod(&Stop[1], &Stop);

  GEOPOINT c; c.Longitude = CenterX; c.Latitude = CenterY;

  while(fabs(EndBearing-StartBearing) > 7.5) {
    if(StartBearing >= 360)
      StartBearing -= 360;
    if(StartBearing < 0)
      StartBearing += 360;

    //	  if (bFillMode)	// Trig calcs not needed on first pass
    {
      FindLatitudeLongitude(c,
                            StartBearing, Radius,
                            &TempPoint);
    }
    AddPoint(airspace_database, &TempPoint, &TempArea.NumPoints,
             NumberOfAirspacePoints);

    StartBearing += Rotation *5 ;
  }

  //  if (bFillMode)	// Trig calcs not needed on first pass
  {
    FindLatitudeLongitude(c, EndBearing, Radius,
                          &TempPoint);
  }
  AddPoint(airspace_database, &TempPoint, &TempArea.NumPoints,
           NumberOfAirspacePoints);
}

static void
CalculateArc(AirspaceDatabase &airspace_database, TCHAR *Text,
             unsigned &NumberOfAirspacePoints)
{
  GEOPOINT Start;
  GEOPOINT End;
  double StartBearing;
  double EndBearing;
  double Radius;
  TCHAR *Comma = NULL;

  ReadCoords(&Text[3],&Start.Longitude , &Start.Latitude);

  Comma = _tcschr(Text,',');
  if(!Comma)
    return;

  ReadCoords(&Comma[1],&End.Longitude , &End.Latitude);

  GEOPOINT c; c.Longitude = CenterX; c.Latitude = CenterY;

  DistanceBearing(c, Start,
                  &Radius, &StartBearing);
  EndBearing = Bearing(c, End);
  TempPoint.Latitude  = Start.Latitude;
  TempPoint.Longitude = Start.Longitude;
  AddPoint(airspace_database, &TempPoint, &TempArea.NumPoints,
           NumberOfAirspacePoints);

  while(fabs(EndBearing-StartBearing) > 7.5) {
    StartBearing += Rotation *5 ;

    if(StartBearing > 360)
      StartBearing -= 360;
    if(StartBearing < 0)
      StartBearing += 360;

    if (bFillMode) { // Trig calcs not needed on first pass
      GEOPOINT c; c.Longitude = CenterX; c.Latitude = CenterY;
      FindLatitudeLongitude(c, StartBearing, Radius,
                            &TempPoint);
    }

    AddPoint(airspace_database, &TempPoint, &TempArea.NumPoints,
             NumberOfAirspacePoints);
  }

  TempPoint  = End;
  AddPoint(airspace_database, &TempPoint, &TempArea.NumPoints,
           NumberOfAirspacePoints);
}


static void
ScanAirspaceCircleBounds(AirspaceDatabase &airspace_database, int i,
                         double bearing)
{
  AIRSPACE_CIRCLE &circle = airspace_database.AirspaceCircle[i];
  GEOPOINT loc;

  FindLatitudeLongitude(circle.Location,
                        bearing, circle.Radius,
                        &loc);

  circle.bounds.minx = min(loc.Longitude, circle.bounds.minx);
  circle.bounds.maxx = max(loc.Longitude, circle.bounds.maxx);
  circle.bounds.miny = min(loc.Latitude, circle.bounds.miny);
  circle.bounds.maxy = max(loc.Latitude, circle.bounds.maxy);
}

void
FindAirspaceCircleBounds(AirspaceDatabase &airspace_database)
{
  for (unsigned i = 0; i < airspace_database.NumberOfAirspaceCircles; ++i) {
    AIRSPACE_CIRCLE &circle = airspace_database.AirspaceCircle[i];

    circle.bounds.minx = circle.Location.Longitude;
    circle.bounds.maxx = circle.Location.Longitude;
    circle.bounds.miny = circle.Location.Latitude;
    circle.bounds.maxy = circle.Location.Latitude;
    ScanAirspaceCircleBounds(airspace_database, i, 0);
    ScanAirspaceCircleBounds(airspace_database, i, 90);
    ScanAirspaceCircleBounds(airspace_database, i, 180);
    ScanAirspaceCircleBounds(airspace_database, i, 270);
    circle.WarningLevel = 0; // clear warnings to initialise

    // JMW detect airspace that wraps across 180
    if ((circle.bounds.minx< -90) && (circle.bounds.maxx>90)) {
      double tmp = circle.bounds.minx;
      circle.bounds.minx = circle.bounds.maxx;
      circle.bounds.maxx = tmp;
    }
  }
}

void
FindAirspaceAreaBounds(AirspaceDatabase &airspace_database)
{
  for (unsigned i = 0; i < airspace_database.NumberOfAirspaceAreas; ++i) {
    AIRSPACE_AREA &area = airspace_database.AirspaceArea[i];
    bool first = true;

    for (unsigned j = area.FirstPoint;
         j < area.FirstPoint + area.NumPoints; j++) {
      const AIRSPACE_POINT &point = airspace_database.AirspacePoint[j];

      if (first) {
        area.bounds.minx = point.Longitude;
        area.bounds.maxx = point.Longitude;
        area.bounds.miny = point.Latitude;
        area.bounds.maxy = point.Latitude;
        first = false;
      } else {
        area.bounds.minx = min(point.Longitude, area.bounds.minx);
        area.bounds.maxx = max(point.Longitude, area.bounds.maxx);
        area.bounds.miny = min(point.Latitude, area.bounds.miny);
        area.bounds.maxy = max(point.Latitude, area.bounds.maxy);
      }
    }

    // JMW detect airspace that wraps across 180
    if ((area.bounds.minx< -90) && (area.bounds.maxx>90)) {
      double tmp = area.bounds.minx;
      area.bounds.minx = area.bounds.maxx;
      area.bounds.maxx = tmp;
      for (unsigned j = area.FirstPoint;
           j < area.FirstPoint + area.NumPoints; ++j) {
        AIRSPACE_POINT &point = airspace_database.AirspacePoint[j];
        if (point.Longitude < 0)
          point.Longitude += 360;
      }
    }

    area.WarningLevel = 0; // clear warnings to initialise
  }
}

bool
ReadAirspace(AirspaceDatabase &airspace_database, const char *path)
{
  ZZIP_FILE *fp = zzip_fopen(path, "rt");
  if (fp == NULL)
    return false;

  ReadAirspace(airspace_database, fp);
  zzip_fclose(fp);
  return true;
}

#ifndef NDEBUG
static void
DumpAirspaceFile(const AirspaceDatabase &airspace_database)
{
  FILE *fp = _tfopen(TEXT("XCSoarAirspace.dmp"), TEXT("wt"));
  if (fp == NULL)
    return;

  airspace_database.Dump(fp);
  fclose(fp);
}
#endif

/**
 * Sorts the airspaces by priority
 */
void SortAirspace(AirspaceDatabase &airspace_database) {
  StartupStore(TEXT("SortAirspace\n"));

  // force acknowledgement before sorting
  ClearAirspaceWarnings(airspace_database, true, false);

  airspace_database.Sort();
}

