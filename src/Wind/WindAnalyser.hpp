/***********************************************************************
**
**   WindAnalyser.h
**
**   This file is part of Cumulus.
**
************************************************************************
**
**   Copyright (c):  2002 by Andr� Somers
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
#include "Util/StaticArray.hpp"

struct MoreData;

/**
 * The windanalyser analyses the list of flightsamples looking
 * for windspeed and direction.
 * @author Andr� Somers
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
  //we are counting the number of circles, the first onces are probably not very round
  int circleCount;
  //true=left, false=right
  bool circleLeft;
  //active is set to true or false by the slot_newFlightMode slot
  bool active;
  int circleDeg;
  Angle last_track;
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

  StaticArray<WindSample, MAXWINDSAMPLES> windsamples;

public:
  WindStore windstore;

  WindAnalyser();

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
  void slot_newFlightMode(const NMEAInfo &info, const DerivedInfo &derived,
                          bool left, int marker);

  /**
   * Called if a new sample is available in the samplelist.
   */
  void slot_newSample(const MoreData &info, DerivedInfo &derived);

  // used to update output if altitude changes
  void slot_Altitude(const MoreData &info, DerivedInfo &derived);

  void slot_newEstimate(const MoreData &info, DerivedInfo &derived,
      Vector v, int quality);

  //void calcThermalDrift();

private:
  void _calcWind(const MoreData &info, DerivedInfo &derived);
};

#endif
