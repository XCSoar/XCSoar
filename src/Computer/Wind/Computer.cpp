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

#include "Computer.hpp"
#include "Settings.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

void
WindComputer::Reset()
{
  circling_wind.Reset();
  wind_ekf.Reset();
  wind_store.reset();
  ekf_active = false;
}

gcc_pure
static double
GetVTakeoffFallback(const GlidePolar &glide_polar)
{
  return glide_polar.IsValid()
    ? glide_polar.GetVTakeoff()
    /* if there's no valid polar, assume 10 m/s (36 km/h); that's an
       arbitrary value, but better than nothing */
    : 10.;
}

void
WindComputer::Compute(const WindSettings &settings,
                      const GlidePolar &glide_polar,
                      const MoreData &basic, DerivedInfo &calculated)
{
  if (!settings.IsAutoWindEnabled()) {
    calculated.estimated_wind_available.Clear();
    return;
  }

  if (!calculated.flight.flying)
    return;

  if (settings.CirclingWindEnabled()) {
    CirclingWind::Result result = circling_wind.NewSample(basic, calculated);
    if (result.IsValid())
      wind_store.SlotMeasurement(basic, result.wind, result.quality);
  }

  if (settings.ZigZagWindEnabled() &&
      basic.airspeed_available && basic.airspeed_real) {
    if (basic.true_airspeed > GetVTakeoffFallback(glide_polar)) {
      WindEKFGlue::Result result = wind_ekf.Update(basic, calculated);
      if (result.quality > 0) {
        wind_store.SlotMeasurement(basic, result.wind, result.quality);

        /* skip WindStore if EKF is used because EKF is already
           filtered */
        /* note that even though we don't use WindStore to obtain the
           wind estimate, we still store the EKF wind vector to it for
           the analysis dialog */
        calculated.estimated_wind = result.wind;
        calculated.estimated_wind_available.Update(basic.clock);
        ekf_active = true;
      }
    }
  } else
    /* EKF cannot be used without airspeed */
    ekf_active = false;

  /* skip WindStore? */
  if (!ekf_active)
    wind_store.SlotAltitude(basic, calculated);
}

void
WindComputer::ComputeHeadWind(const NMEAInfo &basic, DerivedInfo &info)
{
  if (info.wind_available) {
    // If any wind information available
    // .. calculate headwind from given wind information

    info.head_wind =
      (info.wind.bearing - basic.attitude.heading).fastcosine()
      * info.wind.norm;
    info.head_wind_available.Update(basic.clock);
  } else {
    // No information available that let us calculate the head wind
    info.head_wind_available.Clear();
  }
}

void
WindComputer::Select(const WindSettings &settings,
                     const NMEAInfo &basic, DerivedInfo &calculated)
{
  if (calculated.estimated_wind_available.Modified(settings.manual_wind_available)) {
    // auto wind when available and newer than manual wind
    calculated.wind = calculated.estimated_wind;
    calculated.wind_available = calculated.estimated_wind_available;
    calculated.wind_source = ekf_active
      ? DerivedInfo::WindSource::EKF
      : DerivedInfo::WindSource::CIRCLING;

  } else if (settings.external_wind &&
             basic.external_wind_available.Modified(settings.manual_wind_available)) {
    // external wind available
    calculated.wind = basic.external_wind;
    calculated.wind_available = basic.external_wind_available;
    calculated.wind_source = DerivedInfo::WindSource::EXTERNAL;

  } else if (settings.manual_wind_available) {
    // wind was set manually
    calculated.wind = settings.manual_wind;
    calculated.wind_available = settings.manual_wind_available;
    calculated.wind_source = DerivedInfo::WindSource::MANUAL;

  } else {
    // no wind available
    calculated.wind_available.Clear();
    calculated.wind_source = DerivedInfo::WindSource::NONE;
  }
}
