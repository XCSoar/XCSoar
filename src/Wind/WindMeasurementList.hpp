/***********************************************************************
**
**   WindMeasurementList.h
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

#ifndef WINDMEASUREMENTLIST_H
#define WINDMEASUREMENTLIST_H

#include "Vector.hpp"

/**
 * Structure to hold a single wind measurement
 */
struct WindMeasurement
{
  Vector vector;                /**< Wind speed vector */
  int quality;                  /**< Quality of fit */
  long time;                    /**< Time of fix */
  fixed altitude;               /**< Altitude of fix */
};

/** maximum number of windmeasurements in the list. */
#define MAX_MEASUREMENTS 200

/**
 * The WindMeasurementList is a list that can contain and
 * process windmeasurements.
 * @author André Somers
 */
class WindMeasurementList
{
public:
  WindMeasurementList();
  ~WindMeasurementList();

  /**
   * Returns the weighted mean windvector over the stored values, or 0
   * if no valid vector could be calculated (for instance: too little or
   * too low quality data).
   */
  const Vector getWind(fixed Time, fixed alt, bool *found) const;

  /** Adds the windvector vector with quality quality to the list. */
  void addMeasurement(fixed Time, Vector vector, fixed alt, int quality);

protected:
  WindMeasurement *measurementlist[MAX_MEASUREMENTS];
  unsigned int nummeasurementlist;

  /**
   * getLeastImportantItem is called to identify the item that should be
   * removed if the list is too full. Reimplemented from LimitedList.
   */
  unsigned int getLeastImportantItem(fixed Time);
};

#endif
