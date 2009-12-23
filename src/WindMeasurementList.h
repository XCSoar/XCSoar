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

/**
 * The WindMeasurementList is a list that can contain and
 * process windmeasurements.
 * @author André Somers
 */

#include "Vector.h"

struct WindMeasurement
{
  Vector vector;
  int quality;
  long time;
  double altitude;
};

/** maximum number of windmeasurements in the list. */
#define MAX_MEASUREMENTS 200

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
  const Vector getWind(double Time, double alt, bool *found) const;

  /** Adds the windvector vector with quality quality to the list. */
  void addMeasurement(double Time, Vector vector, double alt, int quality);

protected:
  WindMeasurement *measurementlist[MAX_MEASUREMENTS];
  unsigned int nummeasurementlist;

  /**
   * getLeastImportantItem is called to identify the item that should be
   * removed if the list is too full. Reimplemented from LimitedList.
   */
  virtual unsigned int getLeastImportantItem(double Time);

 private:
};

#endif
