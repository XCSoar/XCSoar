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

#include "Airspace/Airspaces.hpp"
#include "Interface.hpp"
#include "Dialogs/Message.hpp"
#include "Language.hpp"
#include "UtilsText.hpp"
#include "LogFile.hpp"
#include "Math/Earth.hpp"
#include "Math/Units.h"
#include "options.h"
#include "Compatibility/string.h"
#include "Math/Geometry.hpp"

#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"

#include <math.h>
#include <tchar.h>
#include <ctype.h>
#include <assert.h>

static bool StartsWith(TCHAR *Text, const TCHAR *LookFor);
static bool ReadCoords(TCHAR *Text, GEOPOINT &point);
static void ReadAltitude(TCHAR *Text, AIRSPACE_ALT *Alt);

static void
CalculateArc(TCHAR *Text);

static void
CalculateSector(TCHAR *Text);

static bool
ParseLine(Airspaces &airspace_database, int nLineType);

static int GetNextLine(ZZIP_FILE *fp, TCHAR *Text);

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

// this can now be called multiple times to load several airspaces.

static TCHAR TempString[READLINE_LENGTH+1];


struct TempAirspaceType {
  TempAirspaceType() {
    reset();
  }
  tstring Name;
  std::vector<GEOPOINT> points;
  int Rotation;
  GEOPOINT Center;
  AirspaceClass_t Type;
  fixed Radius;
  AIRSPACE_ALT Base;
  AIRSPACE_ALT Top;
  bool Waiting;

  void reset() {
    Type = OTHER;
    points.erase(points.begin(), points.end());
    Center.Longitude = fixed_zero;
    Center.Latitude = fixed_zero;
    Rotation = 1;
    Radius = 0;
    Waiting = true;
  }

  void AddPolygon(Airspaces &airspace_database) {
    AbstractAirspace *as = new AirspacePolygon(points);
    as->set_properties(Name, Type, Base, Top);
    airspace_database.insert(as);
  }
  
  void AddCircle(Airspaces &airspace_database) {
    AbstractAirspace *as = new AirspaceCircle(Center, Radius);
    as->set_properties(Name, Type, Base, Top);
    airspace_database.insert(as);
  }
};

static TempAirspaceType temp_area;

static bool
ReadAirspace(Airspaces &airspace_database, ZZIP_FILE *fp)
{
  StartupStore(TEXT("ReadAirspace\n"));
  int	Tock = 0;
  DWORD	dwStep;
  DWORD	dwPos;
  DWORD	dwOldPos = 0L;
  int	nLineType;

  LineCount = 0;

  XCSoarInterface::CreateProgressDialog(gettext(TEXT("Loading Airspace File...")));
  // Need step size finer than default 10
  XCSoarInterface::SetProgressStepSize(5);
  dwStep = zzip_file_size(fp) / 5L;

  dwOldPos = 0L;
  XCSoarInterface::StepProgressDialog();

  temp_area.reset();

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

    if (!ParseLine(airspace_database, nLineType)) {
      return false;
    }

  }

  // Process final area (if any). bFillMode is true.  JG 10-Nov-2005
  if (!temp_area.Waiting) 
    temp_area.AddPolygon(airspace_database);

  return true;
}

static bool
ParseLine(Airspaces &airspace_database, int nLineType)
{
  int nIndex;
  GEOPOINT TempPoint;

  switch (nLineType) {
  case k_nLtAC:
    if (!temp_area.Waiting) {
      temp_area.AddPolygon(airspace_database);
    }
    temp_area.reset();
    
    for (nIndex = 0; nIndex < k_nAreaCount; nIndex++) {
      if (StartsWith(&TempString[3], k_strAreaStart[nIndex])) {
        temp_area.Type = (AirspaceClass_t)k_nAreaType[nIndex];
        break;
      }
    }
    temp_area.Waiting = false;
    break;

  case k_nLtAN:
    temp_area.Name = &TempString[3];
    break;
  case k_nLtAL:
    ReadAltitude(&TempString[3], &temp_area.Base);
    break;
  case k_nLtAH:
    ReadAltitude(&TempString[3],&temp_area.Top);
    break;
  case k_nLtV:
    // Need to set these while in count mode, or DB/DA will crash
    if (StartsWith(&TempString[2], _T("X=")) ||
        StartsWith(&TempString[2], _T("x="))) {
      if (ReadCoords(&TempString[4],temp_area.Center))
        break;
    } else if (StartsWith(&TempString[2], _T("D=-")) ||
               StartsWith(&TempString[2], _T("d=-"))) {
      temp_area.Rotation = -1;
      break;
    } else if (StartsWith(&TempString[2], _T("D=+")) ||
             StartsWith(&TempString[2], _T("d=+"))) {
      temp_area.Rotation = +1;
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
    if (!ReadCoords(&TempString[3],TempPoint))
      goto OnError;
    temp_area.points.push_back(TempPoint);
    break;

  case k_nLtDB:
    CalculateArc(TempString);
    break;

  case k_nLtDA:
    CalculateSector(TempString);
    break;

  case k_nLtDC:
    temp_area.Radius = _tcstod(&TempString[2], NULL)* NAUTICALMILESTOMETRES;
    temp_area.AddCircle(airspace_database);
    temp_area.reset();
    break;

  default:
    break;
  }

  return(true);

OnError:

  TCHAR sTmp[MAX_PATH];
  _stprintf(sTmp, TEXT("%s: %d\r\n\"%s\"\r\n%s."),
            gettext(TEXT("Parse Error at Line")),
            LineCount, TempString,
            gettext(TEXT("Line skipped.")));
  if (MessageBoxX(sTmp, gettext(TEXT("Airspace")),
                  MB_OKCANCEL) == IDCANCEL){
    return(false);
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
        _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
                  gettext(_T("Parse Error at Line")),
                  LineCount, TempString,
                  gettext(_T("Line skipped.")));
        if (MessageBoxX(sTmp,
                        gettext(_T("Airspace")),
                        MB_OKCANCEL) == IDCANCEL)
          return(-1);
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
        _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
                  gettext(_T("Parse Error at Line")),
                  LineCount, TempString,
                  gettext(_T("Line skipped.")));
        if (MessageBoxX(sTmp,
                        gettext(_T("Airspace")),
                        MB_OKCANCEL) == IDCANCEL)
          return(-1);
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
      _stprintf(sTmp, _T("%s: %d\r\n\"%s\"\r\n%s."),
                gettext(_T("Parse Error at Line")),
                LineCount, TempString,
                gettext(_T("Line skipped.")));
      if (MessageBoxX(sTmp, gettext(_T("Airspace")),
                      MB_OKCANCEL) == IDCANCEL)
        return(-1);
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
}

static bool ReadCoords(TCHAR *Text, GEOPOINT &point)
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

  point.Latitude = Ysec/3600 + Ymin/60 + Ydeg;

  if (*Stop == ' ')
    Stop++;

  if (*Stop =='\0') goto OnError;
  if ((*Stop == 'S') || (*Stop == 's'))
    point.Latitude = -point.Latitude;

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

  point.Longitude = Xsec/3600 + Xmin/60 + Xdeg;

  if (*Stop == ' ')
    Stop++;
  if (*Stop =='\0') goto OnError;
  if((*Stop == 'W') || (*Stop == 'w'))
    point.Longitude = -point.Longitude;
  AngleLimit360(point.Longitude);

  return(true);

OnError:
  return(false);

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

  pToken = _tcstok_r(Text, TEXT(" "), &pWClast);

  Alt->Altitude = 0;
  Alt->FL = 0;
  Alt->AGL = 0;
  Alt->Base = abUndef;

  while((pToken != NULL) && (*pToken != '\0')){

    if (isdigit(*pToken)) {
      double d = (double)_tcstod(pToken, &Stop);
      if (Alt->Base == abFL) {
        Alt->FL = d;
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
    }

    else if (_tcscmp(pToken, TEXT("UNL")) == 0) {
      // JMW added Unlimited (used by WGC2008)
      Alt->Base = abMSL;
      Alt->AGL = -1;
      Alt->Altitude = 50000;
    }

    pToken = _tcstok_r(NULL, TEXT(" \t"), &pWClast);

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
CalculateSector(TCHAR *Text)
{
  fixed Radius;
  fixed StartBearing;
  fixed EndBearing;
  TCHAR *Stop;
  GEOPOINT TempPoint;
  static const fixed fixed_75 = fixed(7.5);
  static const fixed fixed_5 = fixed(5);

  Radius = NAUTICALMILESTOMETRES * _tcstod(&Text[2], &Stop);
  StartBearing = _tcstod(&Stop[1], &Stop);
  EndBearing = _tcstod(&Stop[1], &Stop);

  if (EndBearing<StartBearing) {
    EndBearing += fixed(360);
  }

  while(fabs(EndBearing-StartBearing) > fixed_75) {
    StartBearing = AngleLimit360(StartBearing);
    FindLatitudeLongitude(temp_area.Center, AngleLimit360(StartBearing), Radius,
                          &TempPoint);
    temp_area.points.push_back(TempPoint);
    StartBearing += temp_area.Rotation * fixed_5 ;
  }
  FindLatitudeLongitude(temp_area.Center, EndBearing, Radius,
                        &TempPoint);
  temp_area.points.push_back(TempPoint);
}

static void
CalculateArc(TCHAR *Text)
{
  GEOPOINT Start;
  GEOPOINT End;
  fixed StartBearing;
  fixed EndBearing;
  fixed Radius;
  TCHAR *Comma = NULL;
  GEOPOINT TempPoint;
  static const fixed fixed_75 = fixed(7.5);
  static const fixed fixed_5 = fixed(5);

  ReadCoords(&Text[3],Start);

  Comma = _tcschr(Text,',');
  if(!Comma)
    return;

  ReadCoords(&Comma[1],End);

  DistanceBearing(temp_area.Center, Start, &Radius, &StartBearing);
  EndBearing = Bearing(temp_area.Center, End);
  TempPoint.Latitude  = Start.Latitude;
  TempPoint.Longitude = Start.Longitude;
  temp_area.points.push_back(TempPoint);

  while(fabs(EndBearing-StartBearing) > fixed_75) {
    StartBearing += temp_area.Rotation *fixed_5;
    StartBearing = AngleLimit360(StartBearing);
    FindLatitudeLongitude(temp_area.Center, StartBearing, Radius,
                          &TempPoint);
    temp_area.points.push_back(TempPoint);
  }

  TempPoint  = End;
  temp_area.points.push_back(TempPoint);
}

/*
OLD_TASK
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
  }
*/

bool
ReadAirspace(Airspaces &airspace_database, const char *path)
{
  ZZIP_FILE *fp = zzip_fopen(path, "rt");
  if (fp == NULL)
    return false;

  const bool retval = ReadAirspace(airspace_database, fp);
  zzip_fclose(fp);
  return retval;
}

