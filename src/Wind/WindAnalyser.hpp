/***********************************************************************
**
**   WindAnalyser.h
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
**   $Id$
**
***********************************************************************/

#ifndef WINDANALYSER_H
#define WINDANALYSER_H

#include "Vector.hpp"
#include "Wind/WindStore.hpp"
#include "Navigation/GeoPoint.hpp"

/**
 * The windanalyser analyses the list of flightsamples looking
 * for windspeed and direction.
 * @author André Somers
 */
struct WindSample
{
  Vector v;
  fixed t;
  fixed mag;
};

#define MAXWINDSAMPLES 50

/**
 * Class to provide wind estimates from circling
 */
class WindAnalyser
{
public:
  WindAnalyser();
  ~WindAnalyser();

  WindStore windstore;

  /**
   * Clear as if never flown
   */
  void reset();

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
  void slot_newFlightMode(const NMEA_INFO &info, const DERIVED_INFO &derived,
                          bool left, int marker);

  /**
   * Called if a new sample is available in the samplelist.
   */
  void slot_newSample(const NMEA_INFO &info, DERIVED_INFO &derived);

  // used to update output if altitude changes
  void slot_Altitude(const NMEA_INFO &info, DERIVED_INFO &derived);

  void slot_newEstimate(const NMEA_INFO &info, DERIVED_INFO &derived,
      Vector v, int quality);

  //void calcThermalDrift();

private:
  //we are counting the number of circles, the first onces are probably not very round
  int circleCount;
  //true=left, false=right
  bool circleLeft;
  //active is set to true or false by the slot_newFlightMode slot
  bool active;
  int startmarker;
  int circleDeg;
  Angle lastHeading;
  bool pastHalfway;
  Vector minVector;
  Vector maxVector;
  bool curModeOK;
  bool first;
  int startcircle;

  GeoPoint climbstartpos;
  GeoPoint climbendpos;
  fixed climbstarttime;
  fixed climbendtime;

  WindSample windsamples[MAXWINDSAMPLES];
  int numwindsamples;

private:
  void _calcWind(const NMEA_INFO &info, DERIVED_INFO &derived);
};

#endif
