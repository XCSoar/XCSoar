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
#include "DateTime.hpp"
#include "Navigation/GeoPoint.hpp"
#include "Atmosphere/Pressure.hpp"
#include "FLARM/State.hpp"
#include "Sizes.h"
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

  /** location of aircraft */
  GeoPoint Location;

  Validity TrackBearingAvailable;

  /** track angle in degrees true */
  Angle TrackBearing;

  /** Bearing including wind factor */
  Angle Heading;

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
   * Is a barometric altitude available?
   * @see BaroAltitude
   */
  Validity BaroAltitudeAvailable;

  /**
   * Specifies the last-known source for the barometric altitude.
   * This is used to define a priority of sources.
   */
  enum BaroAltitudeOrigin {
    /**
     * Unknown origin or not available.  This is the initial value.
     */
    BARO_ALTITUDE_UNKNOWN,

    /**
     * Parsed from proprietary Garmin sentence "PGRMZ".
     */
    BARO_ALTITUDE_GARMIN,

    /**
     * Parsed from proprietary Garmin sentence "PGRMZ", emitted by
     * a FLARM.
     */
    BARO_ALTITUDE_FLARM,

    /**
     * Parsed from proprietary EW sentence "PGRMZ".
     */
    BARO_ALTITUDE_EW,

    /**
     * Parsed from proprietary Volkslogger sentence "PGCS1".
     */
    BARO_ALTITUDE_VOLKSLOGGER,

    /**
     * Parsed from proprietary Tasman Instruments sentence "PTAS1".
     */
    BARO_ALTITUDE_TASMAN,

    /**
     * Parsed from proprietary Ilec sentence "PDA1".
     */
    BARO_ALTITUDE_ILEC,

    /**
     * Parsed from proprietary Leonardo sentence "C".
     */
    BARO_ALTITUDE_LEONARDO,

    /**
     * Parsed from proprietary Flytec sentence "VMVABD".
     */
    BARO_ALTITUDE_FLYTEC,

    /**
     * Parsed from proprietary Flymaster sentence "VARIO".
     */
    BARO_ALTITUDE_FLYMASTER,

    /**
     * Parsed from proprietary PosiGraph sentence "GPWIN".
     */
    BARO_ALTITUDE_POSIGRAPH,

    /**
     * Parsed from proprietary LX sentence "LXWP0".
     */
    BARO_ALTITUDE_LX,

    /**
     * Parsed from proprietary Triadis sentence "PGRMZ" (Altair Pro).
     */
    BARO_ALTITUDE_TRIADIS_PGRMZ,

    /**
     * Parsed from proprietary Triadis sentence "PDVDV" (Vega).
     */
    BARO_ALTITUDE_TRIADIS_PDVDV,

    /**
     * Parsed from the Westerboer sentence "PWES0".
     */
    BARO_ALTITUDE_WESTERBOER,

    /**
     * Parsed from the Zander sentence "PZAN1".
     */
    BARO_ALTITUDE_ZANDER,

    /**
     * Parsed from the Cambridge CAI302 sentence "PCAID".
     */
    BARO_ALTITUDE_CAI302_PCAID,

    /**
     * Parsed from the Cambridge CAI302 sentence "!w".
     */
    BARO_ALTITUDE_CAI302_W,
  } BaroAltitudeOrigin, PressureAltitudeOrigin;

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

  /** Energy height excess to slow to best glide speed @author JMW */
  fixed EnergyHeight;

  /** Nav Altitude + Energy height (m) */
  fixed TEAltitude;

  /** Altitude used for navigation (GPS or Baro) */
  fixed NavAltitude;

  /** QNH value available? */
  Validity QNHAvailable;

  /**
   * Troposhere atmosphere model for QNH correction
   */
  AtmosphericPressure pressure;

  //##########
  //   Time
  //##########

  fixed Time; /**< global time (seconds UTC) */

  /** GPS date and time (UTC) */
  BrokenDateTime DateTime;

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

  /** MacCready value of external device (if available) */
  fixed MacCready;

  /** Ballast information of external device (if available) */
  fixed Ballast;

  /** Bugs information of external device (if available) */
  fixed Bugs;

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

  FLARM_STATE flarm;

  /**
   * Sets the "true" barometric altitude (i.e. above NN, not above
   * 1013 hPa).
   */
  void SetBaroAltitudeTrue(enum BaroAltitudeOrigin origin, fixed value, bool needs_qnh) {
    if (!QNHAvailable && needs_qnh) {
      BaroAltitudeAvailable.clear();
      return;
    }

    BaroAltitude = value;
    BaroAltitudeOrigin = origin;
    BaroAltitudeAvailable.update(Time);
  }

  /**
   * Sets the pressure altitude above 1013 hPa.
   */
  void SetPressureAltitude(enum BaroAltitudeOrigin origin, fixed value, bool needs_qnh) {
    if (!QNHAvailable && needs_qnh) {
      PressureAltitudeAvailable.clear();
      return;
    }

    PressureAltitude = value;
    PressureAltitudeOrigin = origin;
    PressureAltitudeAvailable.update(Time);
  }

  /**
   * Sets the barometric altitude from a static pressure sensor.
   */
  void SetStaticPressure(enum BaroAltitudeOrigin origin, fixed value) {
    SetBaroAltitudeTrue(origin, pressure.StaticPressureToQNHAltitude(value), true);
    SetPressureAltitude(origin, pressure.StaticPressureToPressureAltitude(value), false);
  }

  /**
   * Provide a "true" barometric altitude, but only use it if the
   * previous altitude was not present or the same/lower priority.
   */
  void ProvideBaroAltitudeTrue(enum BaroAltitudeOrigin origin, fixed value) {
    if (BaroAltitudeOrigin <= origin)
      SetBaroAltitudeTrue(origin, value, false);

    if (PressureAltitudeOrigin <= origin)
      SetPressureAltitude(origin, pressure.QNHAltitudeToPressureAltitude(value), true);
  }

  /**
   * Provide pressure altitude above 1013 hPa, but only use it if
   * the previous altitude was not present or the same/lower priority.
   */
  void ProvidePressureAltitude(enum BaroAltitudeOrigin origin, fixed value) {
    if (PressureAltitudeOrigin <= origin)
      SetPressureAltitude(origin, value, false);

    if (BaroAltitudeOrigin <= origin)
      SetBaroAltitudeTrue(origin, pressure.PressureAltitudeToQNHAltitude(value), true);
  }

  /**
   * Provide barometric altitude from a static pressure sensor, but
   * only use it if the previous altitude was not present or the
   * same/lower priority.
   */
  void ProvideStaticPressure(enum BaroAltitudeOrigin origin, fixed value) {
    if (BaroAltitudeOrigin <= origin)
      SetStaticPressure(origin, value);
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
   * Provide QNH setting
   * @param qnh as QNH setting
   */
  void ProvideQNHSetting(fixed qnh) {
    pressure.set_QNH(qnh);
    QNHAvailable.update(Time);
  }

  /**
   * Set both true airspeed and indicated airspeed to this value
   * [m/s].  This is used by device drivers when it is not documented
   * whether the airspeed variable is TAS or IAS.
   */
  void ProvideBothAirspeeds(fixed as) {
    IndicatedAirspeed = TrueAirspeed = as;
    AirspeedAvailable.update(Time);
  }

  /**
   * Set both true airspeed and indicated airspeed to two different
   * values [m/s].
   */
  void ProvideBothAirspeeds(fixed ias, fixed tas) {
    IndicatedAirspeed = ias;
    TrueAirspeed = tas;
    AirspeedAvailable.update(Time);
  }

  /**
   * Set the true airspeed [m/s] and derive the indicated airspeed
   * from it, using the specified altitude [m].
   */
  void ProvideTrueAirspeedWithAltitude(fixed tas, fixed altitude) {
    TrueAirspeed = tas;
    IndicatedAirspeed = TrueAirspeed /
      AtmosphericPressure::AirDensityRatio(altitude);
    AirspeedAvailable.update(Time);
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
    TotalEnergyVarioAvailable.update(Time);
  }

  /**
   * Set the barometric netto vario value [m/s].
   */
  void ProvideNettoVario(fixed value) {
    NettoVario = value;
    NettoVarioAvailable.update(Time);
  }

  /**
   * Set the external wind value.
   */
  void ProvideExternalWind(const SpeedVector &value) {
    ExternalWind = value;
    ExternalWindAvailable.update(Time);
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
