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

#ifndef XCSOAR_NMEA_INFO_H
#define XCSOAR_NMEA_INFO_H

#include "GPSState.hpp"
#include "NMEA/Validity.hpp"
#include "NMEA/ExternalSettings.hpp"
#include "NMEA/Acceleration.hpp"
#include "NMEA/Attitude.hpp"
#include "SwitchState.hpp"
#include "Time/BrokenDateTime.hpp"
#include "Geo/GeoPoint.hpp"
#include "Atmosphere/Pressure.hpp"
#include "Atmosphere/Temperature.hpp"
#include "DeviceInfo.hpp"
#include "FLARM/Data.hpp"
#include "Geo/SpeedVector.hpp"

#include <type_traits>

/**
 * A struct that holds all the parsed data read from the connected devices
 */
struct NMEAInfo {
  /**
   * A monotonic wall clock time, in seconds, with an undefined
   * reference.  This may get updated even if the device doesn't send
   * any data.  It is used to update and check the #Validity
   * attributes in this struct.
   */
  double clock;

  /**
   * Is the device alive?  This attribute gets updated each time a
   * NMEA line was successfully parsed.
   */
  Validity alive;

  /**
   * The state of the GPS module.  Most of this attribute is only
   * meaningful when #location_available is set.
   */
  GPSState gps;

  AccelerationState acceleration;

  AttitudeState attitude;

  Validity location_available;

  /** location of aircraft */
  GeoPoint location;

  Validity track_available;

  /** track angle in degrees true */
  Angle track;

  //############
  //   Speeds
  //############

  /**
   * Did #TrueAirspeed and/or #IndicatedAirspeed come from an
   * instrument?  If false, then it was calculated (from ground speed
   * and wind).
   */
  bool airspeed_real;

  Validity ground_speed_available;

  /**
   * Is air speed information available?
   * If not, will be estimated from ground speed and wind estimate
   * @see TrueAirspeed in Aircraft
   */
  Validity airspeed_available;

  /**
   * Speed over ground in m/s
   * @see TrueAirspeed
   * @see IndicatedAirspeed
   */
  double ground_speed;

  /**
   * True air speed (m/s)
   * @see Speed
   * @see IndicatedAirspeed
   */
  double true_airspeed;

  /**
   * Indicated air speed (m/s)
   * @see Speed
   * @see TrueAirspeed
   * @see AirDensityRatio
   */
  double indicated_airspeed;

  //##############
  //   Altitude
  //##############

  Validity gps_altitude_available;

  /** GPS altitude AMSL (m) */
  double gps_altitude;

  /**
   * Static pressure value [Pa].
   */
  AtmosphericPressure static_pressure;
  Validity static_pressure_available;

  /**
   * Pitot pressure value [Pa].
   */
  AtmosphericPressure pitot_pressure;
  Validity pitot_pressure_available;

  /**
   * Dynamic pressure value = pitot_pressure - static_pressure [Pa].
   * Use only to compute indicated airspeed.
   */
  AtmosphericPressure dyn_pressure;
  Validity dyn_pressure_available;

  /**
   * Generic sensor calibration data for linear sensors.
   * Used by some sensors e.g.:
   *   1) pitot pressure offset relative to static_pressure [hPa].
   *   2) outside air temperature sensor
   *   3) battery voltage measurement
   */
  double sensor_calibration_offset;
  double sensor_calibration_factor; // 0 is not a valid factor.
  Validity sensor_calibration_available;

  /**
   * Is a barometric altitude available?
   * @see BaroAltitude
   */
  Validity baro_altitude_available;

  /**
   * Barometric altitude (if available)
   * @see BaroAltitudeAvailable
   * @see Altitude
   */
  double baro_altitude;

  /**
   * Pressure altitude, which is the BaroAltitude with QNH=1013.25 as reference (if available)
   * @see BaroAltitudeAvailable
   * @see Altitude
   */
  double pressure_altitude;
  Validity pressure_altitude_available;

  /**
   * Is the barometric altitude given by a "weak" source?  This is
   * used to clear the PGRMZ barometric altitude when a FLARM is
   * detected, to switch from barometric altitude to pressure
   * altitude.
   */
  bool baro_altitude_weak;

  /**
   * Is the pressure altitude given by a "weak" source?  This is used
   * to choose vendor-specific NMEA sentences over a semi-generic one
   * like PGRMZ.  Needed when Vega is improperly muxed with FLARM, to
   * use Vega's pressure altitude instead of alternating between both.
   */
  bool pressure_altitude_weak;

  //##########
  //   Time
  //##########

  /**
   * Did the device provide a time value?
   */
  Validity time_available;

  /**
   * Global time (seconds after UTC midnight)
   */
  double time;

  /**
   * GPS date and time (UTC).
   *
   * Check BrokenDateTime::IsDatePlausible() before using the date
   * part.
   */
  BrokenDateTime date_time_utc;

  //###########
  //   Vario
  //###########

  /**
   * Is an external gross, non-compensated, vertical speed vario signal present?
   * @see NoncompVario
   */
  Validity noncomp_vario_available;

  /**
   * Is an external total energy vario signal available?
   * @see TotalEnergyVario
   */
  Validity total_energy_vario_available;

  /**
   * Is an external netto vario signal available?
   * @see NettoVario
   */
  Validity netto_vario_available;

  /**
   * Gross, non-compensated aircraft vertical speed (m/s, up positive)
   * @see NoncompVarioAvailable
   */
  double noncomp_vario;

  /**
   * Rate of change of total energy of aircraft (m/s, up positive)
   * @see TotalEnergyVarioAvailable
   */
  double total_energy_vario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   * @see NettoVarioAvailable
   */
  double netto_vario;

  //##############
  //   Settings
  //##############

  /**
   * Settings received from external devices.
   */
  ExternalSettings settings;

  //################
  //   Atmosphere
  //################

  /**
   * Is external wind information available?
   * @see ExternalWindSpeed
   * @see ExternalWindDirection
   */
  Validity external_wind_available;

  /**
   * The wind read from the device.  If ExternalWindAvailable is
   * false, then this value is undefined.
   */
  SpeedVector external_wind;

  /**
   * Is temperature information available?
   * @see OutsideAirTemperature
   */
  bool temperature_available;
  /**
   * Temperature of outside air (if available)
   * @see TemperatureAvailable
   */
  Temperature temperature;

  /**
   * Is heading information available?
   */
  Validity heading_available;

  /**
   * Magnetic Heading (if available)
   */
  Angle heading;

 /**
   * Is Magnetic Variation information available?
   */
  Validity variation_available;

  /**
   * Magnetic Variation (if available)
   */
  Angle variation;

  /**
   * Is humidity information available?
   * @see RelativeHumidity
   */
  bool humidity_available;
  /**
   * Humidity of outside air (if available)
   * @see HumidityAvailable
   */
  double humidity;

  //###########
  //   Other
  //###########

  Validity engine_noise_level_available;
  unsigned engine_noise_level;

  Validity voltage_available;

  /** Battery supply voltage information (if available) */
  double voltage;

  Validity battery_level_available;

  /** Battery supply information in percent (0.0 - 100.0; if available) */
  double battery_level;

  /** Switch state of the user inputs */
  SwitchState switch_state;

  double stall_ratio;
  Validity stall_ratio_available;

  DeviceInfo device;

  /**
   * Information about the "secondary" device, e.g. the GPS connected
   * "behind" the LXNAV V7.
   */
  DeviceInfo secondary_device;

  FlarmData flarm;

  void UpdateClock();

  /**
   * Returns a #BrokenDate referring to the given time stamp (all
   * UTC).  This object's #date_time_utc and #time attributes are used
   * to calculate the date.
   *
   * @param other_time the time stamp (see attribute #time)
   */
  gcc_pure
  BrokenDateTime GetDateTimeAt(double other_time) const;

  bool MovementDetected() const {
    return ground_speed_available && ground_speed > 2;
  }

  /**
   * Sets a fake location, and marks it as "unavailable".  This is
   * used during startup to move the glider symbol to the home
   * waypoint.
   */
  void SetFakeLocation(const GeoPoint &_location, const double _altitude) {
    location = _location;
    location_available.Clear();
    gps_altitude = _altitude;
    gps_altitude_available.Clear();
  }

  void ProvideTime(double time);
  void ProvideDate(const BrokenDate &date);

  /**
   * Provide a "true" barometric altitude, but only use it if the
   * previous altitude was not present or the same/lower priority.
   */
  void ProvideBaroAltitudeTrue(double value) {
    baro_altitude = value;
    baro_altitude_weak = false;
    baro_altitude_available.Update(clock);
  }

  /**
   * Same as ProvideBaroAltitudeTrue(), but don't overwrite a "strong"
   * value.
   */
  void ProvideWeakBaroAltitude(double value) {
    if (baro_altitude_available && !baro_altitude_weak)
      /* don't overwrite "strong" value */
      return;

    baro_altitude = value;
    baro_altitude_weak = true;
    baro_altitude_available.Update(clock);
  }

  /**
   * Clear the barometric altitude value if it is "weak".
   */
  void ClearWeakBaroAltitude() {
    if (baro_altitude_available && baro_altitude_weak)
      baro_altitude_available.Clear();
  }

  /**
   * Provide pressure altitude above 1013 hPa, but only use it if
   * the previous altitude was not present or the same/lower priority.
   */
  void ProvidePressureAltitude(double value) {
    pressure_altitude = value;
    pressure_altitude_weak = false;
    pressure_altitude_available.Update(clock);
  }

  /**
   * Same as ProvidePressureAltitude(), but don't overwrite a "strong"
   * value.
   */
  void ProvideWeakPressureAltitude(double value) {
    if (pressure_altitude_available && !pressure_altitude_weak)
      /* don't overwrite "strong" value */
      return;

    pressure_altitude = value;
    pressure_altitude_weak = true;
    pressure_altitude_available.Update(clock);
  }

  /**
   * Clear the pressure altitude value if it is "weak".
   */
  void ClearWeakPressureAltitude() {
    if (pressure_altitude_available && pressure_altitude_weak)
      pressure_altitude_available.Clear();
  }

  /**
   * Provide barometric altitude from a static pressure sensor, but
   * only use it if the previous altitude was not present or the
   * same/lower priority.
   */
  void ProvideStaticPressure(AtmosphericPressure value) {
    static_pressure = value;
    static_pressure_available.Update(clock);
  }

  /**
   * Provide dynamic pressure from a pitot tube.
   * Use only to compute indicated airspeed when static pressure is known.
   * When both pitot- and dynamic pressure are available use dynamic.
   */
  void ProvideDynamicPressure(AtmosphericPressure value) {
    dyn_pressure = value;
    dyn_pressure_available.Update(clock);
  }

  /**
   * Used to compute indicated airspeed
   * when static pressure is known.
   * Value already includes calibration data.
   */
  void ProvidePitotPressure(AtmosphericPressure value) {
    pitot_pressure = value;
    pitot_pressure_available.Update(clock);
  }

  /**
   * Used only as a config value that should be saved.
   *
   * Currently used for:
   *       battery voltage measurement (multiplier)
   *       offset between the pitot- and the static pressure sensor in hPa (zero).
   *       temperature sensor
   */
  void ProvideSensorCalibration(double value, double offset) {
    sensor_calibration_factor = value;
    sensor_calibration_offset = offset;
    sensor_calibration_available.Update(clock);
  }

  /**
   * Returns the pressure altitude, and falls back to the barometric
   * altitude or the GPS altitude.  The "first" element is false if no
   * altitude is available.  The "second" element contains the
   * altitude value [m] if "first" is true.
   */
  gcc_pure
  std::pair<bool, double> GetAnyAltitude() const {
    return pressure_altitude_available
      ? std::make_pair(true, pressure_altitude)
      : (baro_altitude_available
         ? std::make_pair(true, baro_altitude)
         : (gps_altitude_available
            ? std::make_pair(true, gps_altitude)
            : std::make_pair(false, 0.)));
  }

  /**
   * Set both true airspeed and indicated airspeed to this value
   * [m/s].  This is used by device drivers when it is not documented
   * whether the airspeed variable is TAS or IAS.
   */
  void ProvideBothAirspeeds(double as) {
    indicated_airspeed = true_airspeed = as;
    airspeed_available.Update(clock);
    airspeed_real = true;
  }

  /**
   * Set both true airspeed and indicated airspeed to two different
   * values [m/s].
   */
  void ProvideBothAirspeeds(double ias, double tas) {
    indicated_airspeed = ias;
    true_airspeed = tas;
    airspeed_available.Update(clock);
    airspeed_real = true;
  }

  /**
   * Set the true airspeed [m/s] and derive the indicated airspeed
   * from it, using the specified altitude [m].
   */
  void ProvideTrueAirspeedWithAltitude(double tas, double altitude);

  /**
   * Set the indicated airspeed [m/s] and derive the true airspeed
   * from it, using the specified altitude [m].
   */
  void ProvideIndicatedAirspeedWithAltitude(double ias, double altitude);

  /**
   * Set the true airspeed [m/s] and derive the indicated airspeed
   * from it, using the current altitude.
   */
  void ProvideTrueAirspeed(double tas);

  /**
   * Set the indicated airspeed [m/s] and derive the true airspeed
   * from it, using the current altitude.
   */
  void ProvideIndicatedAirspeed(double ias);

  /**
   * Set the gross, non-compensated, plain-old vertical speed vario value [m/s].
   */
  void ProvideNoncompVario(double value) {
    noncomp_vario = value;
    noncomp_vario_available.Update(clock);
  }

  /**
   * Set the barometric TE vario value [m/s].
   */
  void ProvideTotalEnergyVario(double value) {
    total_energy_vario = value;
    total_energy_vario_available.Update(clock);
  }

  /**
   * Set the barometric netto vario value [m/s].
   */
  void ProvideNettoVario(double value) {
    netto_vario = value;
    netto_vario_available.Update(clock);
  }

  /**
   * Set the external wind value.
   */
  void ProvideExternalWind(const SpeedVector &value) {
    external_wind = value;
    external_wind_available.Update(clock);
  }

  /**
   * Clears all information, start with tabula rasa.
   */
  void Reset();

  /**
   * Check the expiry time of the device connection with the wall
   * clock time.  This should be called from a periodic timer.  The
   * GPS time cannot be used here, because a disconnected device would
   * not update its GPS time.
   */
  void ExpireWallClock();

  /**
   * Check expiry times of all attributes which have a time stamp
   * associated with them.  This should be called after the GPS time
   * stamp has been updated.
   */
  void Expire();

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   *
   * Note that this does not copy calculated values which are managed
   * outside of the NMEA parser.
   */
  void Complement(const NMEAInfo &add);
};

static_assert(std::is_trivial<NMEAInfo>::value, "type is not trivial");

#endif
