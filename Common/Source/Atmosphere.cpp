/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "Atmosphere.h"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Interface.hpp"
#include "Components.hpp"
#include <math.h>
#include <stdlib.h> /* for abs() */

unsigned short CuSonde::last_level=0;
double CuSonde::thermalHeight = 0;
double CuSonde::cloudBase = 0;
double CuSonde::hGround = 0;
double CuSonde::maxGroundTemperature = 25.0;
CuSondeLevel CuSonde::cslevels[CUSONDE_NUMLEVELS];

// TODO accuracy: recalculate thermal index etc if maxGroundTemp changes

void CuSonde::test() {
  /*
  CALCULATED_INFO.Flying= true;
  GPS_INFO.TemperatureAvailable = true;
  GPS_INFO.HumidityAvailable = true;

  int i;
  for (i=0; i<3000; i+= 50) {
    GPS_INFO.Altitude = i+50;
    GPS_INFO.OutsideAirTemperature = 20.0+DALR*0.5*i;
    GPS_INFO.RelativeHumidity = 50;
    updateMeasurements(&GPS_INFO, &CALCULATED_INFO);
  }
  for (i=3000; i>0; i-= 50) {
    GPS_INFO.Altitude = i+50;
    GPS_INFO.OutsideAirTemperature = 20.0+DALR*0.5*i;
    GPS_INFO.RelativeHumidity = 50;
    updateMeasurements(&GPS_INFO, &CALCULATED_INFO);
  }
  */
}

/**
 * Sets the predicted maximum ground temperature to val
 * @param val New predicted maximum ground temperature in degrees C
 */
void CuSonde::setForecastTemperature(double val) {
  maxGroundTemperature= val;

  int level;
  int zlevel=0;

  // set these to invalid, so old values must be overwritten
  cloudBase = -1;
  thermalHeight = -1;

  // iterate through all levels
  for (level=0; level<CUSONDE_NUMLEVELS; level++) {
    // update the ThermalIndex for each level with
    // the new maxGroundTemperature
    cslevels[level].updateThermalIndex((unsigned short)level, false);

    // determine to which level measurements are available
    if (cslevels[level].nmeasurements) {
      zlevel = level;
    }
    if ((cslevels[level].nmeasurements==0)&&(zlevel)) break;
  }

  // iterate through all levels with measurements
  for (level=0; level<=zlevel; level++) {
    // calculate ThermalHeight
    findThermalHeight((unsigned short)level);
    // calculate CloudBase
    findCloudBase((unsigned short)level);
  }
}

/**
 * Adjusts the maximum ground temperature by delta
 * @param delta Degrees C to be added to the maximum ground temperature
 */
void CuSonde::adjustForecastTemperature(double delta) {
  setForecastTemperature(maxGroundTemperature+delta);
}

/**
 * Update the measurements if new level reached
 * @param Basic NMEA_INFO for temperature and humidity
 * @param Calculated DERIVED_INFO for Flying status
 */
void
CuSonde::updateMeasurements(const NMEA_INFO *Basic,
                            const DERIVED_INFO *Calculated)
{
  // if (not flying) nothing to update...
  if (!Calculated->Flying)
    return;

  // if (no temperature or humidity available) nothing to update...
  if (!Basic->TemperatureAvailable ||
      !Basic->HumidityAvailable) {
    return;
  }

  // find appropriate level
  unsigned short level = (unsigned short)(((int)(max(Basic->Altitude,0))) / CUSONDE_HEIGHTSTEP);
  // if (level out of range) cancel update
  if (level>=CUSONDE_NUMLEVELS) {
    return;
  }

  // if (level skipped) cancel update
  if (abs(level-last_level)>1) {
    last_level = level;
    return;
  }

  // if (no level transition yet) wait for transition
  if (abs(level-last_level)==0) {
    // QUESTION TB: no need for next line?!
    last_level = level;
    return;
  }

  // calculate ground height
  terrain.Lock();
  if (terrain.GetMap()) {
    RasterRounding rounding(*terrain.GetMap(),0,0);
    hGround =
      terrain.GetTerrainHeight(Basic->Location, rounding);
  }
  terrain.Unlock();

  // if (going up)
  if (level>last_level) {
    cslevels[level].updateTemps(Basic->RelativeHumidity,
				Basic->OutsideAirTemperature);
    cslevels[level].updateThermalIndex(level);

    if (level>0) {
      findThermalHeight((unsigned short)(level-1));
      findCloudBase((unsigned short)(level-1));
    }

  // if (going down)
  } else {
    // QUESTION TB: why level+1 and not level?
    cslevels[level+1].updateTemps(Basic->RelativeHumidity,
				Basic->OutsideAirTemperature);
    cslevels[level+1].updateThermalIndex((unsigned short)(level+1));

    if (level<CUSONDE_NUMLEVELS-1) {
      findThermalHeight(level);
      findCloudBase(level);
    }
  }

  last_level = level;
}

/**
 * Finds the estimated ThermalHeight based on the given level and the one
 * above
 * @param level Level used for calculation
 */
void CuSonde::findThermalHeight(unsigned short level) {
  if (cslevels[level+1].nmeasurements==0) return;
  if (cslevels[level].nmeasurements==0) return;

  // Delta of ThermalIndex
  double dti = cslevels[level+1].thermalIndex - cslevels[level].thermalIndex;

  // Reset estimated ThermalHeight
  cslevels[level].thermalHeight = -1;

  if (fabs(dti)<1.0e-3) {
    return;
  }

  // ti = dlevel * dti + ti0;
  // (-1.6 - ti0)/dti = dlevel;

  double dlevel = (TITHRESHOLD-cslevels[level].thermalIndex)/dti;
  double dthermalheight = (level+dlevel)*CUSONDE_HEIGHTSTEP;
  if (dlevel>0.0) {
    if (dlevel>1.0) {
      if ((level+2<CUSONDE_NUMLEVELS) && (cslevels[level+2].nmeasurements>0)) {
        // estimated point should be in next level.
        return;
      }
    }

    // set the level thermal height to the calculated value
    cslevels[level].thermalHeight = dthermalheight;

    // set the overall thermal height to the calculated value
    thermalHeight = dthermalheight;

#ifdef DEBUG_CUSONDE
    DebugStore("%g # thermal height \r\n", thermalHeight);
#endif
  }
}

/**
 * Finds the estimated CloudBase based on the given level and the one
 * above
 * @param level Level used for calculation
 */
void CuSonde::findCloudBase(unsigned short level) {
  if (cslevels[level+1].nmeasurements==0) return;
  if (cslevels[level].nmeasurements==0) return;

  double dti = (cslevels[level+1].tempDry-cslevels[level+1].dewpoint)
              -(cslevels[level].tempDry-cslevels[level].dewpoint);

  // Reset estimated CloudBase
  cslevels[level].cloudBase = -1;

  if (fabs(dti)<1.0e-3) {
    return;
  }

  // ti = dlevel * dti + ti0;
  // (-3 - ti0)/dti = dlevel;

  double dlevel = -(cslevels[level].tempDry-cslevels[level].dewpoint)/dti;
  double dcloudbase = (level+dlevel)*CUSONDE_HEIGHTSTEP;
  if (dlevel>0.0) {
    if (dlevel>1.0) {
      if ((level+2<CUSONDE_NUMLEVELS) && (cslevels[level+2].nmeasurements>0)) {
        // estimated point should be in next level.
        return;
      }
    }

    // set the level cloudbase to the calculated value
    cslevels[level].cloudBase = dcloudbase;

    // set the overall cloudbase to the calculated value
    cloudBase = dcloudbase;

#ifdef DEBUG_CUSONDE
    DebugStore("%g # cloud base \r\n", cloudBase);
#endif
  }
}

/**
 * Calculates the dew point and saves the measurement
 * @param rh Humidity in percent
 * @param t Temperature in degrees C
 */
void CuSondeLevel::updateTemps(double rh, double t)
{
   double logEx, adewpoint;

   logEx=0.66077+7.5*t/(237.3+t)+(log10(rh)-2);
   adewpoint = (logEx - 0.66077)*237.3/(0.66077+7.5-logEx);

   // QUESTION TB: if(0) ??????
   // update statistics
   if (0) {
     nmeasurements++;
     dewpoint = (adewpoint+dewpoint*(nmeasurements-1))/nmeasurements;
     airTemp = (t+airTemp*(nmeasurements-1))/nmeasurements;
   } else {
     if (nmeasurements==0) {
       dewpoint = adewpoint;
       airTemp = t;
     } else {
       dewpoint = adewpoint*0.5+dewpoint*0.5;
       airTemp = t*0.5+airTemp*0.5;
     }

     nmeasurements++;
   }
}

/**
 * Calculates the ThermalIndex for the given height level
 *
 * ThermalIndex is the difference in dry temp and environmental temp
 * at the specified altitude.
 * @param level level = altitude / CUSONDE_HEIGHTSTEP
 * @param newdata Function logs data to debug file if true
 */
void CuSondeLevel::updateThermalIndex(unsigned short level,
				      bool newdata) {
  double hlevel = level*CUSONDE_HEIGHTSTEP;

  // Calculate the dry temperature at altitude = hlevel
  tempDry = DALR*(hlevel-CuSonde::hGround)+CuSonde::maxGroundTemperature;

  // Calculate ThermalIndex
  thermalIndex = airTemp-tempDry;

#ifdef DEBUG_CUSONDE
  if (newdata) {
    DebugStore("%g %g %g %g # temp measurement \r\n",
	    hlevel, airTemp, dewpoint, thermalIndex);
  }
#endif
}

/*
   - read sensor values
   - calculate dewpoint
   - update statistical model:

   - for each height:
       -- calculate temp of dry parcel of air from ground temp
             projected up at DALR
       -- thermal index is difference in dry temp and environmental temp
       -- calculate dew point
       -- extrapolate difference between DALR temp and dew point to
            find cloud base (where difference is 0)
       -- extrapolate thermal index to find estimated convection height
            (where thermal index = -3)
       -- extrapolation should occur at top band but may not if tow was
           higher than thermal height, or if climbing above cloud base
           (e.g. from wave lift)

   - summary:
       -- cloud base
       -- usable thermal height
       -- estimated thermal strength

   - complications:
       -- terrain
       -- temp higher in thermals?


DALR = -0.00974 degrees C per meter

C to Kelvin = +273.15

 */
