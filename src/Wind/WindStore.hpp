/***********************************************************************
**
**   WindStore.h
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

#ifndef WINDSTORE_H
#define WINDSTORE_H

#include "Wind/WindMeasurementList.hpp"

struct NMEA_INFO;
struct DERIVED_INFO;

/**
 * WindStore receives single windmeasurements and stores these. It uses
 * single measurements to provide a mean value, differentiated for altitude.
 *
 * @author André Somers
 */
class WindStore
{

public:
  WindStore();
  ~WindStore();

public:
  // Public slots
  /**
   * Called with new measurements. The quality is a measure for how good the
   * measurement is. Higher quality measurements are more important in the
   * end result and stay in the store longer.
   */
  void SlotMeasurement(const NMEA_INFO &info, DERIVED_INFO &derived,
      Vector windvector, int quality);

  /**
   * Called if the altitude changes.
   * Determines where measurements are stored and may result in a NewWind
   * signal.
   */
  void SlotAltitude(const NMEA_INFO &info, DERIVED_INFO &derived);

  // signals
  /**
   * Send if a new wind vector has been established. This may happen as
   * new measurements flow in, but also if the altitude changes.
   */
  void NewWind(const NMEA_INFO &info, DERIVED_INFO &derived, Vector& wind);

  const Vector GetWind(fixed Time, fixed h, bool &found) const;

  /**
   * Clear as if never flown
   */
  void reset();
private:
  Vector _lastWind;
  fixed _lastAltitude;
  WindMeasurementList * windlist;

  /**
   * Recalculates the wind from the stored measurements.
   * May result in a NewWind signal.
   */
  void recalculateWind(const NMEA_INFO &info, DERIVED_INFO &derived);

  bool updated;
};

#endif
