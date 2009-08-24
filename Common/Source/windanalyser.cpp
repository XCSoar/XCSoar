/***********************************************************************
**
**   windanalyser.cpp
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
/*
NOTE: Some portions copyright as above

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

#include "windanalyser.h"
#include "Math/FastMath.h"
#include "LogFile.hpp"

#include <stdlib.h>

/*
  About Windanalysation

  Currently, the wind is being analyzed by finding the minimum and the maximum
  groundspeeds measured while flying a circle. The direction of the wind is taken
  to be the direction in wich the speed reaches it's maximum value, the speed
  is half the difference between the maximum and the minimum speeds measured.
  A quality parameter, based on the number of circles allready flown (the first
  circles are taken to be less accurate) and the angle between the headings at
  minimum and maximum speeds, is calculated in order to be able to weigh the
  resulting measurement.

  There are other options for determining the windspeed. You could for instance
  add all the vectors in a circle, and take the resuling vector as the windspeed.
  This is a more complex method, but because it is based on more heading/speed
  measurements by the GPS, it is probably more accurate. If equiped with
  instruments that pass along airspeed, the calculations can be compensated for
  changes in airspeed, resulting in better measurements. We are now assuming
  the pilot flies in perfect circles with constant airspeed, wich is of course
  not a safe assumption.
  The quality indication we are calculation can also be approched differently,
  by calculating how constant the speed in the circle would be if corrected for
  the windspeed we just derived. The more constant, the better. This is again
  more CPU intensive, but may produce better results.

  Some of the errors made here will be averaged-out by the WindStore, wich keeps
  a number of windmeasurements and calculates a weighted average based on quality.
*/

#ifndef NDEBUG
#define DEBUG_WIND
#endif


WindAnalyser::WindAnalyser()
{
  //initialisation
  active=false;
  circleLeft=false;
  circleCount=0;
  startmarker=0;
  circleDeg = 0;
  lastHeading = 0;
  pastHalfway=false;

  minSatCnt = 1; // JMW conf->getWindMinSatCount();
  curModeOK=false;
}

WindAnalyser::~WindAnalyser(){
}


double Magnitude(Vector v) {
  double d = sqrt(v.x*v.x+v.y*v.y);
  return d;
}


/** Called if a new sample is available in the samplelist. */
void WindAnalyser::slot_newSample(NMEA_INFO *nmeaInfo,
                                  DERIVED_INFO *derivedInfo){

  if (!active) return; //only work if we are in active mode


  Vector curVector;

  bool fullCircle=false;

  //circledetection
  if( lastHeading )
    {
      int diff= (int)nmeaInfo->TrackBearing - lastHeading;

      if( diff > 180 )
        diff -= 360;
      if( diff < -180 )
        diff += 360;

      diff = abs(diff);
      circleDeg += diff;
    }
  lastHeading = (int)nmeaInfo->TrackBearing;

  if(circleDeg >= 360 )
    {
      //full circle made!

      fullCircle=true;
      circleDeg = 0;
      circleCount++;  //increase the number of circles flown (used
      //to determine the quality)
    }

  curVector.x= nmeaInfo->Speed*cos(nmeaInfo->TrackBearing*3.14159/180.0);
  curVector.y= nmeaInfo->Speed*sin(nmeaInfo->TrackBearing*3.14159/180.0);

  windsamples[numwindsamples].v = curVector;
  windsamples[numwindsamples].t = nmeaInfo->Time;
  windsamples[numwindsamples].mag = Magnitude(curVector);
  if (numwindsamples<MAXWINDSAMPLES-1) {
    numwindsamples++;
  } else {
    // TODO code: give error, too many wind samples
    // or use circular buffer
  }

  if ((nmeaInfo->Speed< Magnitude(minVector))||first)
    {
      minVector.x = curVector.x; minVector.y = curVector.y;
    }
  if ((nmeaInfo->Speed> Magnitude(maxVector))||first)
    {
      maxVector.x = curVector.x; maxVector.y = curVector.y;
    }

  if (fullCircle) { //we have completed a full circle!
    if (numwindsamples<MAXWINDSAMPLES-1) {
      _calcWind(nmeaInfo, derivedInfo);    //calculate the wind for
                                           //this circle, only if it
                                           //is valid
    }
    fullCircle=false;

    // should set each vector to average
    Vector v;
    v.x = (maxVector.x-minVector.x)/2;
    v.y = (maxVector.y-minVector.y)/2;

    minVector.x = v.x; minVector.y = v.y;
    maxVector.x = v.x; maxVector.y = v.y;

    first = true;
    numwindsamples = 0;

    if (startcircle>1) {
      startcircle--;
    }

    if (startcircle==1) {
      climbstartpos.x = nmeaInfo->Longitude;
      climbstartpos.y = nmeaInfo->Latitude;
      climbstarttime = nmeaInfo->Time;
      startcircle = 0;
    }
    climbendpos.x = nmeaInfo->Longitude;
    climbendpos.y = nmeaInfo->Latitude;
    climbendtime = nmeaInfo->Time;

    //no need to reset fullCircle, it will automaticly be reset in the next itteration.
  }

  first = false;
  windstore.slot_Altitude(nmeaInfo, derivedInfo);
}


void WindAnalyser::slot_Altitude(NMEA_INFO *nmeaInfo,
                                 DERIVED_INFO *derivedInfo) {
  windstore.slot_Altitude(nmeaInfo, derivedInfo);
}



/** Called if the flightmode changes */
void WindAnalyser::slot_newFlightMode(NMEA_INFO *nmeaInfo,
                                      DERIVED_INFO *derivedInfo,
                                      bool left, int marker){
    active=false;  //we are inactive by default
    circleCount=0; //reset the circlecounter for each flightmode
		   //change. The important thing to measure is the
		   //number of turns in this thermal only.

    startcircle = 3; // ignore first two circles in thermal drift calcs

    circleDeg = 0;
    if (derivedInfo->Circling) {
      if (left) {
        circleLeft=true;
        curModeOK=true;
      } else {
        circleLeft=false;
        curModeOK=true;
      }
    } else {

      // end circling?
      if (curModeOK) {
        //        calcThermalDrift();
      }
      curModeOK=false;

      return; //ok, so we are not circling. Exit function.
    }

    //

    //do we have enough satelites in view?
    //    if (satCnt<minSatCnt) return;

    //initialize analyser-parameters
    startmarker=marker;
    startheading= (int)nmeaInfo->TrackBearing;
    active=true;
    first = true;
    numwindsamples = 0;
}


double angleDiff(Vector a, Vector b) {
  double a1;
  double a2;
  double c;
  a1 = atan2(a.y,a.x)*180.0/3.141592;
  a2 = atan2(b.y,b.x)*180.0/3.141592;
  c = a1-a2;
  while (c<-180) {
    c+= 360;
  }
  while (c>180) {
    c-= 360;
  }
  return c;
}


void WindAnalyser::_calcWind(NMEA_INFO *nmeaInfo,
                             DERIVED_INFO *derivedInfo) {
  int i;
  double av=0;
  if (!numwindsamples) return;

  // reject if average time step greater than 2.0 seconds
  if ((windsamples[numwindsamples-1].t - windsamples[0].t)/
      (numwindsamples-1)>2.0) {
    return;
  }

  // find average
  for (i=0; i<numwindsamples; i++) {
    av += windsamples[i].mag;
  }
  av/= numwindsamples;

  // find zero time for times above average
  double rthisp;
  int j;
  int ithis = 0;
  double rthismax = 0;
  double rthismin = 0;
  int jmax= -1;
  int jmin= -1;
  double rpoint;
  int idiff;

  for (j=0; j<numwindsamples; j++) {

    rthisp= 0;
    rpoint = windsamples[j].mag;

    for (i=0; i<numwindsamples; i++) {
      if (i== j) continue;
      ithis = (i+j)%numwindsamples;
      idiff = i;
      if (idiff>numwindsamples/2) {
        idiff = numwindsamples-idiff;
      }
      rthisp += (windsamples[ithis].mag)*idiff;
    }
    if ((rthisp<rthismax)||(jmax==-1)) {
      rthismax = rthisp;
      jmax = j;
    }
    if ((rthisp>rthismin)||(jmin==-1)) {
      rthismin = rthisp;
      jmin = j;
    }
  }

  // jmax is the point where most wind samples are below
  // jmin is the point where most wind samples are above

  maxVector = windsamples[jmax].v;
  minVector = windsamples[jmin].v;

  // attempt to fit cycloid

  double phase;
  double mag = 0.5*(windsamples[jmax].mag - windsamples[jmin].mag);
  double wx, wy;
  double cmag;
  double rthis=0;
  for (i=0; i<numwindsamples; i++) {
    phase = ((i+jmax)%numwindsamples)*3.141592*2.0/numwindsamples;
    wx = cos(phase)*av+mag;
    wy = sin(phase)*av;
    cmag = sqrt(wx*wx+wy*wy)-windsamples[i].mag;
    rthis += cmag*cmag;
  }
  rthis/= numwindsamples;
  rthis = sqrt(rthis);

  int quality;

  if (mag>1) {
    quality = 5- iround(rthis/mag*3);
  } else {
    quality = 5- iround(rthis);
  }

  if (circleCount<3) quality--;
  if (circleCount<2) quality--;
  if (circleCount<1) return;

  quality= min(quality,5);  //5 is maximum quality, make sure we honour that.

  Vector a;

  a.x = -mag*maxVector.x/windsamples[jmax].mag;
  a.y = -mag*maxVector.y/windsamples[jmax].mag;

  if (quality<1) {
    return;   //measurment quality too low
  }

  if (a.x*a.x+a.y*a.y<30*30) {
    // limit to reasonable values (60 knots), reject otherwise
    slot_newEstimate(nmeaInfo, derivedInfo, a, quality);
  }

}


void WindAnalyser::slot_newEstimate(NMEA_INFO *nmeaInfo,
                                    DERIVED_INFO *derivedInfo,
                                    Vector a, int quality)
{

#ifdef DEBUG_WIND
  const char *type;

  if (quality>=6) {
    type = "external wind";
  } else {
    type = "wind circling";
  }
  DebugStore("%f %f %d # %s\n", a.x, a.y, quality, type);
#endif

  windstore.slot_measurement(nmeaInfo, derivedInfo, a, quality);
}


