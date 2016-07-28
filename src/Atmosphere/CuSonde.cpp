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

#include "Atmosphere/CuSonde.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Temperature.hpp"

#include <math.h>
#include <stdlib.h> /* for abs() */
#include <algorithm>

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

using std::max;

void
CuSonde::Reset()
{
  last_level = 0;
  thermal_height = 0;
  cloud_base = 0;
  ground_height = 0;
  max_ground_temperature = Temperature::FromCelsius(25);

  for (unsigned i = 0; i < NUM_LEVELS; ++i)
    cslevels[i].Reset();
}

// TODO accuracy: recalculate thermal index etc if maxGroundTemp changes

/**
 * Sets the predicted maximum ground temperature to val
 * @param val New predicted maximum ground temperature in K
 */
void
CuSonde::SetForecastTemperature(Temperature val)
{
  if (max_ground_temperature == val)
    return;

  max_ground_temperature = val;

  unsigned zlevel = 0;

  // set these to invalid, so old values must be overwritten
  cloud_base = -1;
  thermal_height = -1;

  // iterate through all levels
  auto h_agl = -CuSonde::ground_height;
  for (unsigned level = 0; level < NUM_LEVELS; level++, h_agl += HEIGHT_STEP) {
    // update the ThermalIndex for each level with
    // the new max_ground_temperature
    cslevels[level].UpdateThermalIndex(h_agl, max_ground_temperature);

    // determine to which level measurements are available
    if (!cslevels[level].empty())
      zlevel = level;

    if (cslevels[level].empty() && zlevel)
      break;
  }

  // iterate through all levels with measurements
  for (unsigned level = 0; level <= zlevel; level++) {
    // calculate ThermalHeight
    FindThermalHeight((unsigned short)level);
    // calculate CloudBase
    FindCloudBase((unsigned short)level);
  }
}

/**
 * Update the measurements if new level reached
 * @param basic NMEA_INFO for temperature and humidity
 */
void
CuSonde::UpdateMeasurements(const NMEAInfo &basic,
                            const DerivedInfo &calculated)
{
  // if (not flying) nothing to update...
  if (!calculated.flight.flying)
    return;

  // if (no temperature or humidity available) nothing to update...
  if (!basic.temperature_available)
    return;

  // find appropriate level
  const auto any_altitude = basic.GetAnyAltitude();
  if (!any_altitude.first)
    return;

  unsigned short level = (unsigned short)((int)max(any_altitude.second,
                                                   0.0)
                                          / HEIGHT_STEP);

  // if (level out of range) cancel update
  if (level >= NUM_LEVELS)
    return;

  // if (level skipped) cancel update
  if (abs(level - last_level) > 1) {
    last_level = level;
    return;
  }

  // if (no level transition yet) wait for transition
  if (abs(level - last_level) == 0)
    return;

  // calculate ground height
  ground_height = calculated.altitude_agl;

  // if (going up)
  if (level > last_level) {
    // we round down (level) because of potential lag of temp sensor
    cslevels[level].UpdateTemps(basic.humidity_available,
                                basic.humidity,
                                basic.temperature);

    auto h_agl = level * HEIGHT_STEP - ground_height;
    cslevels[level].UpdateThermalIndex(h_agl, max_ground_temperature);

    if (level > 0) {
      FindThermalHeight((unsigned short)(level - 1));
      FindCloudBase((unsigned short)(level - 1));
    }

  // if (going down)
  } else {
    // we round up (level+1) because of potential lag of temp sensor
    cslevels[level + 1].UpdateTemps(basic.humidity_available,
                                    basic.humidity,
                                    basic.temperature);

    auto h_agl = (level + 1) * HEIGHT_STEP - ground_height;
    cslevels[level + 1].UpdateThermalIndex(h_agl, max_ground_temperature);

    if (level < NUM_LEVELS - 1) {
      FindThermalHeight(level);
      FindCloudBase(level);
    }
  }

  last_level = level;
}

/**
 * Finds the estimated ThermalHeight based on the given level and the one
 * above
 * @param level Level used for calculation
 */
void
CuSonde::FindThermalHeight(unsigned short level)
{
  if (cslevels[level + 1].empty())
    return;
  if (cslevels[level].empty())
    return;

  // Delta of ThermalIndex
  auto dti = cslevels[level + 1].thermal_index - cslevels[level].thermal_index;

  // Reset estimated ThermalHeight
  cslevels[level].thermal_height = -1;

  if (dti.Absolute() < Temperature::FromKelvin(0.001))
    return;

  // ti = dlevel * dti + ti0;
  // (-1.6 - ti0)/dti = dlevel;

  auto dlevel = (TITHRESHOLD - cslevels[level].thermal_index.ToKelvin()) / dti.ToKelvin();
  auto dthermalheight = (level + dlevel) * HEIGHT_STEP;

  if (dlevel > 1
      && (level + 2u < NUM_LEVELS)
      && !cslevels[level + 2].empty())
      // estimated point should be in next level.
      return;

  if (dlevel > 0) {
    // set the level thermal height to the calculated value
    cslevels[level].thermal_height = dthermalheight;

    // set the overall thermal height to the calculated value
    thermal_height = dthermalheight;
  }
}

/**
 * Finds the estimated CloudBase based on the given level and the one
 * above
 * @param level Level used for calculation
 */
void
CuSonde::FindCloudBase(unsigned short level)
{
  if (cslevels[level + 1].dewpoint_empty())
    return;
  if (cslevels[level].dewpoint_empty())
    return;

  auto dti = (cslevels[level + 1].dry_temperature - cslevels[level + 1].dewpoint)
               - (cslevels[level].dry_temperature - cslevels[level].dewpoint);

  // Reset estimated CloudBase
  cslevels[level].cloud_base = -1;

  if (dti.Absolute() < Temperature::FromKelvin(0.001))
    return;

  // ti = dlevel * dti + ti0;
  // (-3 - ti0)/dti = dlevel;

  auto dlevel = -(cslevels[level].dry_temperature - cslevels[level].dewpoint).ToKelvin() / dti.ToKelvin();
  auto dcloudbase = (level + dlevel) * HEIGHT_STEP;

  if (dlevel > 1
      && (level + 2u < NUM_LEVELS)
      && !cslevels[level + 2].empty())
    // estimated point should be in next level.
    return;

  if (dlevel > 0) {
    // set the level cloudbase to the calculated value
    cslevels[level].cloud_base = dcloudbase;

    // set the overall cloudbase to the calculated value
    cloud_base = dcloudbase;
  }
}

/**
 * Calculates the dew point and saves the measurement
 * @param rh Humidity in percent
 * @param t Temperature in K
 */
void
CuSonde::Level::UpdateTemps(bool humidity_valid, double humidity, Temperature temperature)
{
  if (humidity_valid)
  {
    auto log_ex = 7.5 * temperature.ToCelsius() / (237.3 + temperature.ToCelsius()) +
              (log10(humidity) - 2);
    auto _dewpoint = Temperature::FromCelsius(log_ex * 237.3 / (7.5 - log_ex));

    if (dewpoint_empty())
      dewpoint = _dewpoint;
    else
      dewpoint = (_dewpoint + dewpoint) / 2;

    has_dewpoint = true;
  }

  // update statistics
  if (empty())
    air_temperature = temperature;
  else
    air_temperature = (temperature + air_temperature) / 2;

  has_data = true;
}

/**
 * Calculates the ThermalIndex for the given height level
 *
 * ThermalIndex is the difference in dry temp and environmental temp
 * at the specified altitude.
 * @param level level = altitude / HEIGHT_STEP
 * @param newdata Function logs data to debug file if true
 */
void
CuSonde::Level::UpdateThermalIndex(double h_agl,
                                   Temperature max_ground_temperature)
{
  // Calculate the dry temperature at altitude = hlevel
  dry_temperature = max_ground_temperature + Temperature::FromKelvin(DALR * h_agl);

  // Calculate ThermalIndex
  thermal_index = air_temperature - dry_temperature;
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
