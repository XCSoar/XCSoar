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

// TODO
// - check buffer size in LongditudeToString and LattiditudeToString
// - convertion function
// - fill up UnitDescriptors with convertion factors
// - registry re-store
// - unit dialog support


//default       EU   UK   US   AUS
//altitude      m    ft   ft   m
//verticalspeed m/s  kts  kts  kts
//wind speed    km/  kts  mp   kts
//IAS           km/  kts  mp   kts
//distance      km   nm   ml   nm


#include "stdafx.h"

#include <stdio.h>
//#include <assert.h>

#include "units.h"
#include "externs.h"

CoordinateFormats_t Units::CoordinateFormat;

UnitDescriptor_t Units::UnitDescriptors[] ={
  {NULL,         1,          0},
  {TEXT("km"),   0.001,      0},
  {TEXT("nm"),   0.00053996, 0},
  {TEXT("sm"),   0.0006214,  0},
  {TEXT("km/h"), 0.0036,     0},
  {TEXT("kn"),   0.001944,   0},
  {TEXT("mph"),  0.002237,   0},
  {TEXT("m/s"),  1.0,        0},
  {TEXT("fpm"),  3.281/60.0, 0},
  {TEXT("m"),    1.0,        0},
  {TEXT("ft"),   3.281,      0},
  {TEXT("K"),    1,          0},
  {TEXT("°C"),   1.0,       -273.15},
  {TEXT("°F"),   1.8,       -459.67}
};

Units_t Units::UserDistanceUnit = unKiloMeter;
Units_t Units::UserAltitudeUnit = unMeter;
Units_t Units::UserHorizontalSpeedUnit = unKiloMeterPerHour;
Units_t Units::UserVerticalSpeedUnit = unMeterPerSecond;
Units_t Units::UserWindSpeedUnit = unKiloMeterPerHour;

bool Units::LongditudeToString(double Longitude, TCHAR *Buffer, size_t size){

  TCHAR EW[] = TEXT("WE");
  int dd, mm, ss;

  int sign = Longitude<0 ? 0 : 1;
  Longitude = fabs(Longitude);


  switch(CoordinateFormat){
    case cfDDMMSS:
      dd = (int)Longitude;
      Longitude = (Longitude - dd) * 60.0;
      mm = (int)(Longitude);
      Longitude = (Longitude - mm) * 60.0;
      ss = (int)(Longitude + 0.5);
      if (ss >= 60)
        mm++;
      if (mm >= 60)
        dd++;
      _stprintf(Buffer, TEXT("%c%03d°%02d'%02d\""), EW[sign], dd, mm, ss);
    break;
    case cfDDMMSSss:
      dd = (int)Longitude;
      Longitude = (Longitude - dd) * 60.0;
      mm = (int)(Longitude);
      Longitude = (Longitude - mm) * 60.0;
      _stprintf(Buffer, TEXT("%c%03d°%02d'%05.2f\""), EW[sign], dd, mm, Longitude);
    break;
    case cfDDMMmmm:
      dd = (int)Longitude;
      Longitude = (Longitude - dd) * 60.0;
      _stprintf(Buffer, TEXT("%c%03d°%06.3f'"), EW[sign], dd, Longitude);
    break;
    case cfDDdddd:
      _stprintf(Buffer, TEXT("%c%08.4f°"), EW[sign], Longitude);
    break;
    default:
//      assert(false /* undefined coordinateformat */);
    break;
  }

  return(true);

}


bool Units::LattitudeToString(double Lattitude, TCHAR *Buffer, size_t size){

  TCHAR EW[] = TEXT("SN");
  int dd, mm, ss;

  int sign = Lattitude<0 ? 0 : 1;
  Lattitude = fabs(Lattitude);


  switch(CoordinateFormat){
    case cfDDMMSS:
      dd = (int)Lattitude;
      Lattitude = (Lattitude - dd) * 60.0;
      mm = (int)(Lattitude);
      Lattitude = (Lattitude - mm) * 60.0;
      ss = (int)(Lattitude + 0.5);
      if (ss >= 60)
        mm++;
      if (mm >= 60)
        dd++;
      _stprintf(Buffer, TEXT("%c%02d°%02d'%02d\""), EW[sign], dd, mm, ss);
    break;
    case cfDDMMSSss:
      dd = (int)Lattitude;
      Lattitude = (Lattitude - dd) * 60.0;
      mm = (int)(Lattitude);
      Lattitude = (Lattitude - mm) * 60.0;
      _stprintf(Buffer, TEXT("%c%02d°%02d'%05.2f\""), EW[sign], dd, mm, Lattitude);
    break;
    case cfDDMMmmm:
      dd = (int)Lattitude;
      Lattitude = (Lattitude - dd) * 60.0;
      _stprintf(Buffer, TEXT("%c%02d°%06.3f'"), EW[sign], dd, Lattitude);
    break;
    case cfDDdddd:
      _stprintf(Buffer, TEXT("%c%07.4f°"), EW[sign], Lattitude);
    break;
    default:
//      assert(false /* undefined coordinateformat */);
    break;
  }

  return(true);

}

TCHAR *Units::GetUnitName(Units_t Unit){
  return(gettext(UnitDescriptors[Unit].Name));
}

Units_t Units::GetUserDistanceUnit(void){
  return(UserDistanceUnit);
}

Units_t Units::SetUserDistanceUnit(Units_t NewUnit){
  Units_t last = UserDistanceUnit;
  if (UserDistanceUnit != NewUnit){
    UserDistanceUnit = NewUnit;
    NotifyUnitChanged();
  }
  return(last);
}

Units_t Units::GetUserAltitudeUnit(void){
  return(UserAltitudeUnit);
}

Units_t Units::SetUserAltitudeUnit(Units_t NewUnit){
  Units_t last = UserAltitudeUnit;
  if (UserAltitudeUnit != NewUnit){
    UserAltitudeUnit = NewUnit;
    NotifyUnitChanged();
  }
  return(last);
}

Units_t Units::GetUserHorizontalSpeedUnit(void){
  return(UserHorizontalSpeedUnit);
}
Units_t Units::SetUserHorizontalSpeedUnit(Units_t NewUnit){
  Units_t last = UserHorizontalSpeedUnit;
  if (UserHorizontalSpeedUnit != NewUnit){
    UserHorizontalSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return(last);
}

Units_t Units::GetUserVerticalSpeedUnit(void){
  return(UserVerticalSpeedUnit);
}
Units_t Units::SetUserVerticalSpeedUnit(Units_t NewUnit){
  Units_t last = UserVerticalSpeedUnit;
  if (UserVerticalSpeedUnit != NewUnit){
    UserVerticalSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return(last);
}

Units_t Units::GetUserWindSpeedUnit(void){
  return(UserWindSpeedUnit);
}
Units_t Units::SetUserWindSpeedUnit(Units_t NewUnit){
  Units_t last = UserWindSpeedUnit;
  if (UserWindSpeedUnit != NewUnit){
    UserWindSpeedUnit = NewUnit;
    NotifyUnitChanged();
  }
  return(last);
}

void Units::NotifyUnitChanged(void){
  // todo

  if (SPEEDMODIFY==TOMPH) {
    SetUserHorizontalSpeedUnit(unStatuteMilesPerHour);
  }
  if (SPEEDMODIFY==TOKNOTS) {
    SetUserHorizontalSpeedUnit(unKnots);
  }
  if (SPEEDMODIFY==TOKPH) {
    SetUserHorizontalSpeedUnit(unKiloMeterPerHour);
  }

  if (DISTANCEMODIFY == TOMILES) {
    SetUserDistanceUnit(unStatuteMiles);
  }
  if (DISTANCEMODIFY == TONAUTICALMILES) {
    SetUserDistanceUnit(unNauticalMiles);
  }
  if (DISTANCEMODIFY == TOKILOMETER) {
    SetUserDistanceUnit(unKiloMeter);
  }

  if (ALTITUDEMODIFY == TOFEET) {
    SetUserAltitudeUnit(unFeet);
  }
  if (ALTITUDEMODIFY == TOMETER) {
    SetUserAltitudeUnit(unMeter);
  }

  if (LIFTMODIFY==TOKNOTS) {
    SetUserVerticalSpeedUnit(unKnots);
  }
  if (LIFTMODIFY==TOMETER) {
    SetUserVerticalSpeedUnit(unMeterPerSecond);
  }

}

TCHAR *Units::GetHorizontalSpeedName(){
  return(GetUnitName(GetUserHorizontalSpeedUnit()));
}

TCHAR *Units::GetVerticalSpeedName(){
  return(GetUnitName(GetUserVerticalSpeedUnit()));
}

TCHAR *Units::GetDistanceName(){
  return(GetUnitName(GetUserDistanceUnit()));
}

TCHAR *Units::GetAltitudeName(){
  return(GetUnitName(GetUserAltitudeUnit()));
}


bool Units::FormatUserAltitude(double Altitude, TCHAR *Buffer, size_t size){

  int prec;
  TCHAR sTmp[32];
  UnitDescriptor_t *pU = &UnitDescriptors[UserAltitudeUnit];

  Altitude = Altitude * pU->ToUserFact; // + pU->ToUserOffset;

//  prec = 4-log10(Altitude);
//  prec = max(prec, 0);
  prec = 0;

  _stprintf(sTmp, TEXT("%.*f%s"), prec, Altitude, pU->Name);

  if (_tcslen(sTmp) < size-1){
    _tcscpy(Buffer, sTmp);
    return(true);
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size-1] = '\0';
    return(false);
  }

}

bool Units::FormatUserDistance(double Distance, TCHAR *Buffer, size_t size){

  int prec;
  double value;
  TCHAR sTmp[32];
  UnitDescriptor_t *pU = &UnitDescriptors[UserDistanceUnit];

  value = Distance * pU->ToUserFact; // + pU->ToUserOffset;

  if (value >= 100)
    prec = 0;
  else if (value > 10)
    prec = 1;
  else if (value > 1)
    prec = 2;
  else {
    prec = 3;
    if (UserDistanceUnit == unKiloMeter){
      prec = 0;
      pU = &UnitDescriptors[unMeter];
      value = Distance * pU->ToUserFact;
    }
    /* dont know what to do on miles ....
    if (UserDistanceUnit == unNauticalMiles || UserDistanceUnit == unStauteMiles){
      prec = 0;
      pU = &UnitDescriptors[unFeet];
      value = Distance * pU->ToUserFact;
    }
    */
  }

  _stprintf(sTmp, TEXT("%.*f%s"), prec, value, pU->Name);

  if (_tcslen(sTmp) < size-1){
    _tcscpy(Buffer, sTmp);
    return(true);
  } else {
    _tcsncpy(Buffer, sTmp, size);
    Buffer[size-1] = '\0';
    return(false);
  }

}



