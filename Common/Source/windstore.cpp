/***********************************************************************
**
**   windstore.cpp
**
**   This file is part of Cumulus
**
************************************************************************
**
**   Copyright (c):  2002 by André Somers
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id$
**
***********************************************************************/

#include "stdafx.h"

#include "windstore.h"
#include "XCSoar.h"

WindStore::WindStore(NMEA_INFO *thenmeaInfo, DERIVED_INFO *thederivedInfo) {
  //create the lists
  windlist = new WindMeasurementList(thenmeaInfo, thederivedInfo);
  nmeaInfo = thenmeaInfo;
  derivedInfo = thederivedInfo;
  updated = true;
}


WindStore::~WindStore(){
  delete windlist;
}


/**
  * Called with new measurements. The quality is a measure for how
  * good the measurement is. Higher quality measurements are more
  * important in the end result and stay in the store longer.
  */
void WindStore::slot_measurement(Vector windvector, int quality){
  updated = true;
  windlist->addMeasurement(windvector, nmeaInfo->Altitude, quality);
  //we may have a new wind value, so make sure it's emitted if needed!
  recalculateWind();
}


/**
  * Called if the altitude changes.
  * Determines where measurements are stored and may result in a
  * newWind signal.
  */

void WindStore::slot_Altitude(){

  if ((fabs(nmeaInfo->Altitude-_lastAltitude)>100.0)||(updated)) {
    //only recalculate if there is a significant change
    recalculateWind();

    updated = false;
    _lastAltitude=nmeaInfo->Altitude;
  }
}


/** Recalculates the wind from the stored measurements.
  * May result in a newWind signal. */

void WindStore::recalculateWind() {
  bool found;
  Vector CurWind= windlist->getWind(nmeaInfo->Altitude, &found);

  if (found) {
    if ((fabs(CurWind.x-_lastWind.x)>1.0) ||
	(fabs(CurWind.y-_lastWind.y)>1.0) || updated) {
      _lastWind=CurWind;

      updated = false;
      _lastAltitude=nmeaInfo->Altitude;

      newWind(CurWind);
    }
  } // otherwise, don't change anything

}


void WindStore::newWind(Vector &wind) {
  //
  double mag = sqrt(wind.x*wind.x+wind.y*wind.y);
  double bearing = atan2(wind.y, wind.x)*(180/3.14159);
  derivedInfo->WindSpeed = mag;
  if (bearing<0) {
    bearing += 360;
  }
  derivedInfo->WindBearing = bearing;

#ifdef DEBUG
  char Temp[100];
  sprintf(Temp,"%f %f 0 # estimate\n",wind.x,wind.y);
  DebugStore(Temp);
#endif

}
