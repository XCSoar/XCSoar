/***********************************************************************
**
**   windstore.h
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
**   $Id: windstore.h,v 1.2 2005/06/23 12:19:54 jwharington Exp $
**
***********************************************************************/

#ifndef WINDSTORE_H
#define WINDSTORE_H

#include "windmeasurementlist.h"

/**
  * WindStore receives single windmeasurements and stores these. It uses
  * single measurements to provide a mean value, differentiated for altitude.
  *
  * @author André Somers
  */


class WindStore  {

public: 
  WindStore(NMEA_INFO *thenmeaInfo, DERIVED_INFO *thederivedInfo
	    );
  ~WindStore();
  
public: // Public slots
  /**
    * Called with new measurements. The quality is a measure for how good the
    * measurement is. Higher quality measurements are more important in the
    * end result and stay in the store longer.
    */
  void slot_measurement(Vector windvector, int quality);
  /** Called if the altitude changes.
    * Determines where measurements are stored and may result in a newWind
    * signal. */
  void slot_Altitude();

  // signals
  /**
    * Send if a new wind vector has been established. This may happen as
    * new measurements flow in, but also if the altitude changes.
    */
  void newWind(Vector& wind);

private:
  DERIVED_INFO *derivedInfo;
  NMEA_INFO *nmeaInfo;

  Vector _lastWind;
  double _lastAltitude;
  WindMeasurementList * windlist ;

  /** Recalculates the wind from the stored measurements.
    * May result in a newWind signal. */
  void recalculateWind();
  
  bool updated;

};

#endif
