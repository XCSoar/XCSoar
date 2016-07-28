/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Atmosphere/Temperature.hpp"

struct NMEAInfo;
struct DerivedInfo;

/**
 * Namespaces that provides simple estimates of thermal heights from lapse rates
 * derived from temperature trace obtained during flight.
 */
class CuSonde {
public:
  /** Meters between levels */
  static constexpr unsigned HEIGHT_STEP = 100;
  /** Number of levels */
  static constexpr unsigned NUM_LEVELS = 100;

  struct Level {
    /** Environmental temperature in K */
    Temperature air_temperature;
    /** DewPoint in K */
    Temperature dewpoint;
    /** Dry temperature in K */
    Temperature dry_temperature;
    /** ThermalIndex in K */
    Temperature thermal_index;

    void UpdateTemps(bool humidity_valid, double humidity, Temperature temperature);
    void UpdateThermalIndex(double h_agl, Temperature max_ground_temperature);

    /** Has any data */
    bool has_data;
    /** Has dewpoint data */
    bool has_dewpoint;

    /** Estimated ThermalHeight with data of this level */
    double thermal_height;
    /** Estimated CloudBase with data of this level */
    double cloud_base;

    bool empty() const {
      return !has_data;
    }

    bool dewpoint_empty() const {
      return !has_dewpoint;
    }

    void Reset() {
      has_data = false;
      has_dewpoint = false;
    }
  };

  /** Expected temperature maximum on the ground */
  Temperature max_ground_temperature;
  /** Height of ground above MSL */
  double ground_height;
  unsigned short last_level;
  Level cslevels[NUM_LEVELS];

  /** Estimated ThermailHeight */
  double thermal_height;
  /** Estimated CloudBase */
  double cloud_base;

  void Reset();

  void UpdateMeasurements(const NMEAInfo &basic, const DerivedInfo &calculated);
  void FindCloudBase(unsigned short level);
  void FindThermalHeight(unsigned short level);
  void SetForecastTemperature(Temperature temperature);
};

#endif
