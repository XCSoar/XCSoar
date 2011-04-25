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

#ifndef XCSOAR_NMEA_INFO_H
#define XCSOAR_NMEA_INFO_H

#include "NMEA/Validity.hpp"
#include "NMEA/ExternalSettings.hpp"
#include "DateTime.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Atmosphere/Pressure.hpp"
#include "FLARM/State.hpp"
#include "Engine/Navigation/SpeedVector.hpp"

/**
 * State of external switch devices (esp Vega)
 */
struct SWITCH_INFO
{
  bool AirbrakeLocked;
  bool FlapPositive;
  bool FlapNeutral;
  bool FlapNegative;
  bool GearExtended;
  bool Acknowledge;
  bool Repeat;
  bool SpeedCommand;
  bool UserSwitchUp;
  bool UserSwitchMiddle;
  bool UserSwitchDown;

  enum {
    MODE_UNKNOWN,
    MODE_CIRCLING,
    MODE_CRUISE,
  } FlightMode;

  bool FlapLanding;
  // bool Stall;

  void reset();
};

/**
 * State of GPS fix
 */
struct GPS_STATE
{
  static const unsigned MAXSATELLITES = 12;

  //############
  //   Status
  //############

  /**
   * FixQuality
   * 1 = GPS fix (SPS)
   * 2 = DGPS fix
   * 3 = PPS fix
   * 4 = Real Time Kinematic
   * 5 = Float RTK
   * 6 = estimated (dead reckoning) (2.3 feature)
   * 7 = Manual input mode
   * 8 = Simulation mode
   */
  int FixQuality;

  /** Number of satellites used for gps fix */
  int SatellitesUsed;

  /** GPS Satellite ids */
  int SatelliteIDs[MAXSATELLITES];

  /** Horizontal dilution of precision */
  fixed HDOP;

  /** Is the GPS unit moving? (Speed > 2.0) */
  bool MovementDetected;

  /**
   * Is the fix real? (no replay, no simulator)
   */
  bool real;

  /** Is XCSoar in replay mode? */
  bool Replay;

  /**
   * Did the simulator provide the GPS position?
   */
  bool Simulator;

#ifdef ANDROID
  /**
   * Was this fix obtained from an Android InternalGPS device?  If
   * yes, then link timeout detection is disabled, because we get
   * notifications from Android when the GPS gets disconnected.
   */
  bool AndroidInternalGPS;
#endif

  void reset();
};

/**
 * State of acceleration of aircraft, with calculated pseudo-attitude reference
 */
struct ACCELERATION_STATE
{
  //##################
  //   Acceleration
  //##################

  /** Estimated bank angle */
  Angle BankAngle;
  /** Estimated pitch angle */
  Angle PitchAngle;

  /**
   * Is G-load information available?
   * @see Gload
   */
  bool Available;

  /**
   * G-Load information of external device (if available)
   * or estimated (assuming balanced turn) 
   * @see AccelerationAvailable
   */
  fixed Gload;

  void reset() {
    Available = false;
  }

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   */
  void complement(const ACCELERATION_STATE &add);
};


/**
 * A struct that holds all the parsed data read from the connected devices
 */
struct NMEA_INFO {
  /**
   * Is there a device connected?  This attribute gets updated each
   * time a NMEA line was successfully parsed.  When this expires, it
   * means that the device got disconnected.
   */
  Validity Connected;

  GPS_STATE gps;

  ACCELERATION_STATE acceleration;

  Validity LocationAvailable;

  bool ConnectedAndHasFix() const {
    return Connected && LocationAvailable && (gps.SatellitesUsed>0);
  }

  /** location of aircraft */
  GeoPoint Location;

  Validity TrackBearingAvailable;

  /** track angle in degrees true */
  Angle TrackBearing;

  //############
  //   Speeds
  //############

  Validity GroundSpeedAvailable;

  /**
   * Is air speed information available?
   * If not, will be estimated from ground speed and wind estimate
   * @see TrueAirspeed in Aircraft
   */
  Validity AirspeedAvailable;

  /**
   * Speed over ground in m/s
   * @see TrueAirspeed
   * @see IndicatedAirspeed
   */
  fixed GroundSpeed;

  /**
   * True air speed (m/s)
   * @see Speed
   * @see IndicatedAirspeed
   */
  fixed TrueAirspeed;

  /**
   * Indicated air speed (m/s)
   * @see Speed
   * @see TrueAirspeed
   * @see AirDensityRatio
   */
  fixed IndicatedAirspeed;

  //##############
  //   Altitude
  //##############

  Validity GPSAltitudeAvailable;

  fixed GPSAltitude; /**< GPS altitude AMSL (m) */

  /**
   * Static pressure value [Pa].
   */
  fixed static_pressure;
  Validity static_pressure_available;

  /**
   * Is a barometric altitude available?
   * @see BaroAltitude
   */
  Validity BaroAltitudeAvailable;

  /**
   * Barometric altitude (if available)
   * @see BaroAltitudeAvailable
   * @see Altitude
   */
  fixed BaroAltitude;

  /**
   * Pressure altitude, which is the BaroAltitude with QNH=1013.25 as reference (if available)
   * @see BaroAltitudeAvailable
   * @see Altitude
   */
  fixed PressureAltitude;
  Validity PressureAltitudeAvailable;

  /** Altitude used for navigation (GPS or Baro) */
  fixed NavAltitude;

  //##########
  //   Time
  //##########

  fixed Time; /**< global time (seconds UTC) */

  /** GPS date and time (UTC) */
  BrokenDateTime DateTime;

  /**
   * Is the BrokenDate part of DateTime available?
   */
  bool DateAvailable;

  //###########
  //   Vario
  //###########

  /**
   * Is an external vario signal available?
   * @see TotalEnergyVario
   */
  Validity TotalEnergyVarioAvailable;

  /**
   * Is an external netto vario signal available?
   * @see NettoVario
   */
  Validity NettoVarioAvailable;

  /**
   * Rate of change of total energy of aircraft (m/s, up positive)
   * @see TotalEnergyVarioAvailable
   */
  fixed TotalEnergyVario;

  /**
   * Vertical speed of air mass (m/s, up positive)
   * @see NettoVarioAvailable
   */
  fixed NettoVario;

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
  Validity ExternalWindAvailable;

  /**
   * The wind read from the device.  If ExternalWindAvailable is
   * false, then this value is undefined.
   */
  SpeedVector ExternalWind;

  /**
   * Is temperature information available?
   * @see OutsideAirTemperature
   */
  bool TemperatureAvailable;
  /**
   * Temperature of outside air (if available)
   * @see TemperatureAvailable
   */
  fixed OutsideAirTemperature;

  /**
   * Is humidity information available?
   * @see RelativeHumidity
   */
  bool HumidityAvailable;
  /**
   * Humidity of outside air (if available)
   * @see HumidityAvailable
   */
  fixed RelativeHumidity;

  //###########
  //   Other
  //###########

  Validity engine_noise_level_available;
  unsigned engine_noise_level;

  Validity SupplyBatteryVoltageAvailable;

  /** Battery supply voltage information (if available) */
  fixed SupplyBatteryVoltage;

  bool SwitchStateAvailable;

  /** Switch state of the user inputs */
  SWITCH_INFO SwitchState;

  fixed StallRatio;
  Validity StallRatioAvailable;

  FLARM_STATE flarm;

  /**
   * Sets a fake location, and marks it as "unavailable".  This is
   * used during startup to move the glider symbol to the home
   * waypoint.
   */
  void SetFakeLocation(const GeoPoint &_location, const fixed _altitude) {
    Location = _location;
    LocationAvailable.Clear();
    GPSAltitude = _altitude;
    GPSAltitudeAvailable.Clear();
  }

  /**
   * Provide a "true" barometric altitude, but only use it if the
   * previous altitude was not present or the same/lower priority.
   */
  void ProvideBaroAltitudeTrue(fixed value) {
    BaroAltitude = value;
    BaroAltitudeAvailable.Update(Time);
  }

  /**
   * Provide pressure altitude above 1013 hPa, but only use it if
   * the previous altitude was not present or the same/lower priority.
   */
  void ProvidePressureAltitude(fixed value) {
    PressureAltitude = value;
    PressureAltitudeAvailable.Update(Time);
  }

  /**
   * Provide barometric altitude from a static pressure sensor, but
   * only use it if the previous altitude was not present or the
   * same/lower priority.
   */
  void ProvideStaticPressure(fixed value) {
    static_pressure = value;
    static_pressure_available.Update(Time);
  }

  /**
   * Returns the barometric altitude, and falls back to the GPS
   * altitude.
   */
  fixed GetAltitudeBaroPreferred() const {
    return BaroAltitudeAvailable
      ? BaroAltitude
      : GPSAltitude;
  }

  /**
   * Set both true airspeed and indicated airspeed to this value
   * [m/s].  This is used by device drivers when it is not documented
   * whether the airspeed variable is TAS or IAS.
   */
  void ProvideBothAirspeeds(fixed as) {
    IndicatedAirspeed = TrueAirspeed = as;
    AirspeedAvailable.Update(Time);
  }

  /**
   * Set both true airspeed and indicated airspeed to two different
   * values [m/s].
   */
  void ProvideBothAirspeeds(fixed ias, fixed tas) {
    IndicatedAirspeed = ias;
    TrueAirspeed = tas;
    AirspeedAvailable.Update(Time);
  }

  /**
   * Set the true airspeed [m/s] and derive the indicated airspeed
   * from it, using the specified altitude [m].
   */
  void ProvideTrueAirspeedWithAltitude(fixed tas, fixed altitude) {
    TrueAirspeed = tas;
    IndicatedAirspeed = TrueAirspeed /
      AtmosphericPressure::AirDensityRatio(altitude);
    AirspeedAvailable.Update(Time);
  }

  /**
   * Set the true airspeed [m/s] and derive the indicated airspeed
   * from it, using the current altitude.
   */
  void ProvideTrueAirspeed(fixed tas) {
    ProvideTrueAirspeedWithAltitude(tas, GetAltitudeBaroPreferred());
  }

  /**
   * Set the barometric TE vario value [m/s].
   */
  void ProvideTotalEnergyVario(fixed value) {
    TotalEnergyVario = value;
    TotalEnergyVarioAvailable.Update(Time);
  }

  /**
   * Set the barometric netto vario value [m/s].
   */
  void ProvideNettoVario(fixed value) {
    NettoVario = value;
    NettoVarioAvailable.Update(Time);
  }

  /**
   * Set the external wind value.
   */
  void ProvideExternalWind(const SpeedVector &value) {
    ExternalWind = value;
    ExternalWindAvailable.Update(Time);
  }

  /**
   * Clears all information, start with tabula rasa.
   */
  void reset();

  void ResetFlight(bool full);

  /**
   * Check the expiry time of the device connection with the wall
   * clock time.  This should be called from a periodic timer.  The
   * GPS time cannot be used here, because a disconnected device would
   * not update its GPS time.
   */
  void expire_wall_clock();

  /**
   * Check expiry times of all attributes which have a time stamp
   * associated with them.  This should be called after the GPS time
   * stamp has been updated.
   */
  void expire();

  /**
   * Adds data from the specified object, unless already present in
   * this one.
   *
   * Note that this does not copy calculated values which are managed
   * outside of the NMEA parser.
   */
  void complement(const NMEA_INFO &add);
};

#endif
