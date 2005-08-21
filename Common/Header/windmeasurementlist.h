/***********************************************************************
**
**   windmeasurementlist.h
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
**   $Id: windmeasurementlist.h,v 1.4 2005/08/21 06:52:35 jwharington Exp $
**
***********************************************************************/

#ifndef WINDMEASUREMENTLIST_H
#define WINDMEASUREMENTLIST_H

/**The WindMeasurementList is a list that can contain and process windmeasurements.
  *@author André Somers
  */

#include "Calculations.h"
#include "vector.h"

struct WindMeasurement {
  Vector vector;
  int quality;
  long time;
  double altitude;
};

#define MAX_MEASUREMENTS 200
//maximum number of windmeasurements in the list. No idea what a sensible value would be...

class WindMeasurementList {
public: 
  WindMeasurementList(NMEA_INFO *thenmeaInfo, DERIVED_INFO *thederivedInfo);
  ~WindMeasurementList();

  /**
   * Returns the weighted mean windvector over the stored values, or 0
   * if no valid vector could be calculated (for instance: too little or
   * too low quality data).
   */
  Vector getWind(double alt, bool *found);
  /** Adds the windvector vector with quality quality to the list. */
  void addMeasurement(Vector vector, double alt, int quality);

protected:
  WindMeasurement *measurementlist[MAX_MEASUREMENTS];
  unsigned int nummeasurementlist;

  /**
   * getLeastImportantItem is called to identify the item that should be
   * removed if the list is too full. Reimplemented from LimitedList.
   */
  virtual unsigned int getLeastImportantItem(void);

 private:
  DERIVED_INFO *derivedInfo;
  NMEA_INFO *nmeaInfo;

};

#endif
