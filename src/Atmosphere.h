/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

struct NMEA_INFO;

class CuSondeLevel {
public:
  CuSondeLevel() {
    nmeasurements = 0;
    thermalHeight = -1;
    cloudBase = -1;
  }

  /** Environmental temperature in degrees C */
  double airTemp;
  /** DewPoint in degrees C */
  double dewpoint;
  /** Dry temperature in degrees C */
  double tempDry;
  /** ThermalIndex in degrees C */
  double thermalIndex;

  void updateTemps(double rh, double t);
  void updateThermalIndex(unsigned short level, bool newdata=true);

  /** Number of measurements */
  int nmeasurements;

  /** Estimated ThermalHeight with data of this level */
  double thermalHeight;
  /** Estimated CloudBase with data of this level */
  double cloudBase;
};

/** Meters between levels */
#define CUSONDE_HEIGHTSTEP 100
/** Number of levels */
#define CUSONDE_NUMLEVELS 100
/**
 * Dry adiabatic lapse rate (degrees C per meter)
 *
 * DALR = dT/dz = g/c_p =
 * @see http://en.wikipedia.org/wiki/Lapse_rate#Dry_adiabatic_lapse_rate
 * @see http://pds-atmospheres.nmsu.edu/education_and_outreach/encyclopedia/adiabatic_lapse_rate.htm
 */
#define DALR -0.00974
/** ThermalIndex threshold in degrees C */
#define TITHRESHOLD -1.6

/**
 * Namespaces that provides simple estimates of thermal heights from lapse rates
 * derived from temperature trace obtained during flight.
 */
namespace CuSonde
{
  /** Expected temperature maximum on the ground */
  extern double maxGroundTemperature;
  /** Height of ground above MSL */
  extern double hGround;
  extern unsigned short last_level;
  void updateMeasurements(const NMEA_INFO &basic);
  extern CuSondeLevel cslevels[CUSONDE_NUMLEVELS];
  void findCloudBase(unsigned short level);
  void findThermalHeight(unsigned short level);
  void adjustForecastTemperature(double delta);
  void setForecastTemperature(double val);

  /** Estimated ThermailHeight */
  extern double thermalHeight;
  /** Estimated CloudBase */
  extern double cloudBase;
};

#endif
