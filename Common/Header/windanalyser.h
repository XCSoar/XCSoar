/***********************************************************************
**
**   windanalyser.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002 by André Somers
**
**   This file is distributed under the terms of the General Public
**   Licence. See the file COPYING for more information.
**
**   $Id: windanalyser.h,v 1.4 2005/08/21 06:52:35 jwharington Exp $
**
***********************************************************************/

#ifndef WINDANALYSER_H
#define WINDANALYSER_H

#include "vector.h"

#include "windstore.h"

/**The windanalyser analyses the list of flightsamples looking for windspeed and direction.
  *@author André Somers
  */


class WindSample {
 public:
  Vector v;
  double t;
  double mag;
};


#define MAXWINDSAMPLES 100


class WindAnalyser  {

public:
    WindAnalyser(NMEA_INFO *thenmeaInfo, DERIVED_INFO *thederivedInfo);
    ~WindAnalyser();

    WindStore windstore;

// Signals
    /**
      * Send if a new windmeasurement has been made. The result is included in wind,
      * the quality of the measurement (1-5; 1 is bad, 5 is excellent) in quality.
      */
    void newMeasurement(Vector wind, int quality);

// Public slots
    /**
     * Called if the flightmode changes
     */
    void slot_newFlightMode(bool left, int);
    /**
     * Called if a new sample is available in the samplelist.
     */
    void slot_newSample();

    // used to update output if altitude changes
    void slot_Altitude();

    void slot_newEstimate(Vector v, int quality);

    /**
     * Called if a new satelite constellation has been detected.
     */
    void slot_newConstellation();

    void calcThermalDrift();
private: // Private attributes
    int circleCount; //we are counting the number of circles, the first onces are probably not very round
    bool circleLeft; //true=left, false=right
    bool active;     //active is set to true or false by the slot_newFlightMode slot
    int startmarker;
    int startheading;
    int circleDeg;
    int lastHeading;
    bool pastHalfway;
    Vector minVector;
    Vector maxVector;
    int satCnt;
    int minSatCnt;
    bool curModeOK;
    bool first;
    int startcircle;

    Vector climbstartpos;
    Vector climbendpos;
    double climbstarttime;
    double climbendtime;

    WindSample windsamples[MAXWINDSAMPLES];
    int numwindsamples;

private: // Private memberfunctions
    void _calcWind();
    void _calcWindNew();

  DERIVED_INFO *derivedInfo;
  NMEA_INFO *nmeaInfo;
};

#endif
