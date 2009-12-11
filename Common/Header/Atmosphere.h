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

#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

struct NMEA_INFO;
struct DERIVED_INFO;

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

class CuSonde {
public:
  /** Expected temperature maximum on the ground */
  static double maxGroundTemperature;
  /** Height of ground above MSL */
  static double hGround;
  static unsigned short last_level;
  static void updateMeasurements(const NMEA_INFO &basic,
      const DERIVED_INFO &calculated);
  static CuSondeLevel cslevels[CUSONDE_NUMLEVELS];
  static void findCloudBase(unsigned short level);
  static void findThermalHeight(unsigned short level);
  static void adjustForecastTemperature(double delta);
  static void setForecastTemperature(double val);

  /** Estimated ThermailHeight */
  static double thermalHeight;
  /** Estimated CloudBase */
  static double cloudBase;

  static void test();
};

#endif
