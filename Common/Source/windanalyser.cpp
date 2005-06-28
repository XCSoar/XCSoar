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
**   $Id: windanalyser.cpp,v 1.4 2005/06/28 13:41:04 jwharington Exp $
**
***********************************************************************/

#include "Calculations.h"
#include "windanalyser.h"

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

WindAnalyser::WindAnalyser(NMEA_INFO *thenmeaInfo, DERIVED_INFO *thederivedInfo):windstore(thenmeaInfo,
											   thederivedInfo)
{
  nmeaInfo = thenmeaInfo;
  derivedInfo = thederivedInfo;

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
void WindAnalyser::slot_newSample(){
    if (!active) return; //only work if we are in active mode

    Vector curVector;

    bool fullCircle=false;
    //circledetection
    if( lastHeading )
    {
        int diff= nmeaInfo->TrackBearing - lastHeading;

        if( diff > 180 )
            diff -= 360;
        if( diff < -180 )
            diff += 360;

	diff = abs(diff);
        circleDeg += diff;
    }
    lastHeading = nmeaInfo->TrackBearing;

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

    if ((nmeaInfo->Speed< Magnitude(minVector))||first) 
      {
	minVector.x = curVector.x; minVector.y = curVector.y;
      }
    if ((nmeaInfo->Speed> Magnitude(maxVector))||first) 
      {
	maxVector.x = curVector.x; maxVector.y = curVector.y;
      }

    if (fullCircle) { //we have completed a full circle!
        _calcWind();    //calculate the wind for this circle
        fullCircle=false;

	// should set each vector to average
	Vector v;
	v.x = (maxVector.x-minVector.x)/2;
	v.y = (maxVector.y-minVector.y)/2;

	minVector.x = v.x; minVector.y = v.y;
	maxVector.x = v.x; maxVector.y = v.y;

	first = true;
	    
        //no need to reset fullCircle, it will automaticly be reset in the next itteration.
    } 

    first = false;
    windstore.slot_Altitude();
}


void WindAnalyser::slot_Altitude() {
    windstore.slot_Altitude();
}


/** Called if the flightmode changes */
void WindAnalyser::slot_newFlightMode(bool left, int marker){
    active=false;  //we are inactive by default
    circleCount=0; //reset the circlecounter for each flightmode
		   //change. The important thing to measure is the
		   //number of turns in this thermal only.
    circleDeg = 0;
    if ((derivedInfo->Circling) && (left)) {
        circleLeft=true;
    } else if ((derivedInfo->Circling) && (!left)) {
        circleLeft=false;
    } else {
        curModeOK=false;
        return; //ok, so we are not circling. Exit function.
    }
    //remember that our current mode is ok.
    curModeOK=true;
    //do we have enough satelites in view?
    //    if (satCnt<minSatCnt) return;

    //initialize analyser-parameters
    startmarker=marker;
    startheading= nmeaInfo->TrackBearing;
    active=true;
    first = true;
}


double angleDiff(Vector a, Vector b) {
  double a1;
  double a2;
  double c;
  a1 = atan2(a.y,a.x)*180.0/3.141592;
  a2 = atan2(b.y,b.x)*180.0/3.141592;
  c = a1-a2;
  if (c<-180) {
    c+= 360;
  }
  if (c>180) {
    c-= 360;
  }
  return c;
}


void WindAnalyser::_calcWind() {
  int aDiff= angleDiff(minVector, maxVector);
  int quality;

  double sp, mmax, mmin;
  mmax = Magnitude(maxVector);
  mmin = Magnitude(minVector);
  sp = 0.5*(mmax-mmin);

  /*deterime quality.
    Currently, we are using the question how well the min and the max vectors
    are on oposing sides of the circle to determine the quality. 140 degrees is
    the minimum separation, 180 is ideal.
    Furthermore, the first two circles are considdered to be of lesser quality.
  */
  
  quality=5-((180-abs(aDiff))/8);
  if (sp<2.0) {
    quality=5-((180-abs(aDiff))/16);
  }
  if (circleCount<2) quality--;
  if (circleCount<1) quality--;
  
  if (quality<1) return;   //measurment quality too low

  quality= min(quality,5);  //5 is maximum quality, make sure we honour that.

  Vector a;
  a.x = 0.0;
  a.y = 0.0;

  a.x = -sp*maxVector.x/mmax;
  a.y = -sp*maxVector.y/mmax;

  //take both directions for min and max vector into account
  //create a vector object for the resulting wind
  //the direction of the wind is the direction where the greatest speed occured
  
  //the speed of the wind is half the difference between the minimum and the maximumspeeds.
  //let the world know about our measurement!
  
  windstore.slot_measurement(a, quality);
}

void WindAnalyser::slot_newEstimate(Vector a, int quality)
{

  if (quality==6) {
    quality = 3; // provided externally
  } else {
    quality= min(quality,5);  //5 is maximum quality, make sure we honour that.

    if (circleCount<2) quality--;
    if (circleCount<1) quality--;
    if (quality<1) return;   //measurment quality too low
  }

  windstore.slot_measurement(a, quality);
}

void WindAnalyser::slot_newConstellation() {
  /*
  satCnt=gps->getLastSatInfo().satCount;
  if (active && (satCnt<minSatCnt))  //we are active, but the satcount drops below minimum
  {
    active=false;
    curModeOK=true;
    return;
  }

  if (!active && curModeOK && satCnt>=minSatCnt) { //we are not active because we had low satcount, but that has been rectified so we become active
    //initialize analyser-parameters
    startmarker=calculator->samplelist->at(0)->marker;
    startheading=calculator->samplelist->at(0)->vector.getAngleDeg();
    active=true;
    minVector=calculator->samplelist->at(0)->vector.Clone();
    maxVector=minVector.Clone();
  }
  */
}
