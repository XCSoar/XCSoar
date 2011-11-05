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

#ifndef XCSOAR_ATMOSPHERE_CUSONDE_HPP
#define XCSOAR_ATMOSPHERE_CUSONDE_HPP

#include "Math/fixed.hpp"

struct NMEAInfo;
struct DerivedInfo;

/**
 * Namespaces that provides simple estimates of thermal heights from lapse rates
 * derived from temperature trace obtained during flight.
 */
namespace CuSonde
{
  /** Meters between levels */
  static const unsigned HEIGHT_STEP = 100;
  /** Number of levels */
  static const unsigned NUM_LEVELS = 100;

  struct Level {
    /** Environmental temperature in degrees C */
    fixed airTemp;
    /** DewPoint in degrees C */
    fixed dewpoint;
    /** Dry temperature in degrees C */
    fixed tempDry;
    /** ThermalIndex in degrees C */
    fixed thermalIndex;

    void updateTemps(fixed rh, fixed t);
    void updateThermalIndex(unsigned short level, bool newdata=true);

    /** Number of measurements */
    unsigned nmeasurements;

    /** Estimated ThermalHeight with data of this level */
    fixed thermalHeight;
    /** Estimated CloudBase with data of this level */
    fixed cloudBase;

    bool empty() const {
      return nmeasurements == 0;
    }
  };

  /** Expected temperature maximum on the ground */
  extern fixed maxGroundTemperature;
  /** Height of ground above MSL */
  extern fixed hGround;
  extern unsigned short last_level;
  void updateMeasurements(const NMEAInfo &basic,
                          const DerivedInfo &calculated);
  extern Level cslevels[NUM_LEVELS];
  void findCloudBase(unsigned short level);
  void findThermalHeight(unsigned short level);
  void adjustForecastTemperature(fixed delta);
  void setForecastTemperature(fixed val);

  /** Estimated ThermailHeight */
  extern fixed thermalHeight;
  /** Estimated CloudBase */
  extern fixed cloudBase;
};

#endif
