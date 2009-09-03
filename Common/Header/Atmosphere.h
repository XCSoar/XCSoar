/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

class CuSondeLevel {
public:
  CuSondeLevel() {
    nmeasurements = 0;
    thermalHeight = -1;
    cloudBase = -1;
  }
  double airTemp; // degrees C
  double dewpoint; // degrees C
  double tempDry; // degrees C
  double thermalIndex;
  void updateTemps(double rh, double t);
  void updateThermalIndex(unsigned short level, bool newdata=true);
  int nmeasurements;

  double thermalHeight; // as estimated by this level
  double cloudBase; // as estimated by this level
};


#define CUSONDE_HEIGHTSTEP 100 // meters between levels
#define CUSONDE_NUMLEVELS 100 // number of levels
#define DALR -0.00974 // degrees C per meter
#define TITHRESHOLD -1.6 // thermal index threshold in degrees C

class CuSonde {
public:
  static double maxGroundTemperature;
  static double hGround;
  static unsigned short last_level;
  static void updateMeasurements(const NMEA_INFO *Basic,
                                 const DERIVED_INFO *Calculated);
  static CuSondeLevel cslevels[CUSONDE_NUMLEVELS];
  static void findCloudBase(unsigned short level);
  static void findThermalHeight(unsigned short level);
  static void adjustForecastTemperature(double delta);
  static void setForecastTemperature(double val);

  static double thermalHeight; // as estimated by this level
  static double cloudBase; // as estimated by this level

  static void test();
};

#endif
