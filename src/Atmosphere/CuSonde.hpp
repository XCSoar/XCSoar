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
    /** Environmental temperature in degrees K */
    double airTemp;
    /** DewPoint in degrees K */
    double dewpoint;
    /** Dry temperature in degrees K */
    double tempDry;
    /** ThermalIndex in degrees K */
    double thermalIndex;

    void UpdateTemps(double humidity, double temperature);
    void UpdateThermalIndex(double h_agl, double max_ground_temperature);

    /** Number of measurements */
    unsigned nmeasurements;

    /** Estimated ThermalHeight with data of this level */
    double thermalHeight;
    /** Estimated CloudBase with data of this level */
    double cloudBase;

    bool empty() const {
      return nmeasurements == 0;
    }

    void Reset() {
      nmeasurements = 0;
    }
  };

  /** Expected temperature maximum on the ground */
  double maxGroundTemperature;
  /** Height of ground above MSL */
  double hGround;
  unsigned short last_level;
  Level cslevels[NUM_LEVELS];

  /** Estimated ThermailHeight */
  double thermalHeight;
  /** Estimated CloudBase */
  double cloudBase;

  void Reset();

  void UpdateMeasurements(const NMEAInfo &basic, const DerivedInfo &calculated);
  void FindCloudBase(unsigned short level);
  void FindThermalHeight(unsigned short level);
  void SetForecastTemperature(double val);
};

#endif
