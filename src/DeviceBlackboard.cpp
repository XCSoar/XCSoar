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

#include "DeviceBlackboard.hpp"
#include "Protection.hpp"
#include "Math/Earth.hpp"
#include "UtilsSystem.hpp"
#include "TeamCodeCalculation.h"
#include "FLARM/FlarmDetails.hpp"
#include "Asset.hpp"
#include "Device/Parser.hpp"
#include "Device/List.hpp"
#include "Device/Descriptor.hpp"
#include "Device/All.hpp"
#include "Math/Constants.h"
#include "GlideSolvers/GlidePolar.hpp"
#include "Simulator.hpp"

#include <limits.h>

#ifdef WIN32
#include <windows.h>
#endif

#define fixed_inv_2g fixed(1.0/(2.0*9.81))
#define fixed_inv_g fixed(1.0/9.81)
#define fixed_small fixed(0.001)

DeviceBlackboard device_blackboard;

/**
 * Initializes the DeviceBlackboard
 */
void
DeviceBlackboard::Initialise()
{
  ScopeLock protect(mutexBlackboard);

  // Clear the gps_info and calculated_info
  gps_info.reset();
  calculated_info.reset();
  
  // Set the NAVWarning positive (assume not gps found yet)
  gps_info.gps.NAVWarning = true;
  gps_info.gps.Simulator = false;

#ifdef ANDROID
  gps_info.gps.AndroidInternalGPS = false;
#endif

  // Clear the SwitchStates
  gps_info.SwitchStateAvailable = false;
  gps_info.SwitchState.AirbrakeLocked = false;
  gps_info.SwitchState.FlapPositive = false;
  gps_info.SwitchState.FlapNeutral = false;
  gps_info.SwitchState.FlapNegative = false;
  gps_info.SwitchState.GearExtended = false;
  gps_info.SwitchState.Acknowledge = false;
  gps_info.SwitchState.Repeat = false;
  gps_info.SwitchState.SpeedCommand = false;
  gps_info.SwitchState.UserSwitchUp = false;
  gps_info.SwitchState.UserSwitchMiddle = false;
  gps_info.SwitchState.UserSwitchDown = false;
  gps_info.SwitchState.FlightMode = SWITCH_INFO::MODE_UNKNOWN;

  // Set GPS assumed time to system time
  gps_info.DateTime = BrokenDateTime::NowUTC();
  gps_info.Time = fixed(gps_info.DateTime.hour * 3600 +
                        gps_info.DateTime.minute * 60 +
                        gps_info.DateTime.second);

  if (is_simulator()) {
    #ifdef _SIM_STARTUPSPEED
      gps_info.Speed = _SIM_STARTUPSPEED;
    #endif
    #ifdef _SIM_STARTUPALTITUDE
      gps_info.GPSAltitude = _SIM_STARTUPALTITUDE;
    #endif
  }
}

/**
 * Sets the location and altitude to loc and alt
 *
 * Called at startup when no gps data available yet
 * @param loc New location
 * @param alt New altitude
 */
void
DeviceBlackboard::SetStartupLocation(const GeoPoint &loc, const fixed alt)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().Location = loc;
  SetBasic().GPSAltitude = alt;

  // set NAVWarning flags because this value was not provided
  // by a real GPS
  SetBasic().gps.NAVWarning = true;
}

/**
 * Sets the location, altitude and other basic parameters
 *
 * Used by the IgcReplay
 * @param loc New location
 * @param speed New speed
 * @param bearing New bearing
 * @param alt New altitude
 * @param baroalt New barometric altitude
 * @param t New time
 * @see IgcReplay::UpdateInternal()
 */
void
DeviceBlackboard::SetLocation(const GeoPoint &loc,
                              const fixed speed, const Angle bearing,
                              const fixed alt, const fixed baroalt,
                              const fixed t)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().acceleration.Available = false;
  SetBasic().Location = loc;
  SetBasic().GroundSpeed = speed;
  SetBasic().IndicatedAirspeed = speed; // cheat
  SetBasic().TrackBearing = bearing;
  SetBasic().AirspeedAvailable = false;
  SetBasic().GPSAltitude = alt;
  SetBasic().SetBaroAltitudeTrue(NMEA_INFO::BARO_ALTITUDE_UNKNOWN, baroalt);
  SetBasic().Time = t;
  SetBasic().TotalEnergyVarioAvailable = false;
  SetBasic().NettoVarioAvailable = false;
  SetBasic().ExternalWindAvailable = false;
  SetBasic().gps.Replay = true;
};

/**
 * Stops the replay
 */
void DeviceBlackboard::StopReplay() {
  ScopeLock protect(mutexBlackboard);
  SetBasic().GroundSpeed = fixed_zero;
  SetBasic().gps.Replay = false;
}

/**
 * Sets the NAVWarning to val
 * @param val New value for NAVWarning
 */
void
DeviceBlackboard::SetNAVWarning(bool val)
{
  ScopeLock protect(mutexBlackboard);
  GPS_STATE &gps = SetBasic().gps;
  gps.NAVWarning = val;
  if (!val) {
    // if NavWarning is false, since this is externally forced
    // by the simulator, we also set the number of satelites used
    // as a simulated value
    gps.SatellitesUsed = 6;
  }
}

/**
 * Lowers the connection status of the device
 *
 * Connected + Fix -> Connected + No Fix
 * Connected + No Fix -> Not connected
 * @return True if still connected afterwards, False otherwise
 */
bool
DeviceBlackboard::LowerConnection()
{
  ScopeLock protect(mutexBlackboard);
  GPS_STATE &gps = SetBasic().gps;

  bool enable_link_timeouts = true;
#ifdef ANDROID
  if (gps.AndroidInternalGPS)
    enable_link_timeouts = false;
#endif

  if (enable_link_timeouts && gps.Connected)
    gps.Connected--;

  return gps.Connected > 0;
}

/**
 * Raises the connection status to connected + fix
 */
void
DeviceBlackboard::RaiseConnection()
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().gps.Connected = 2;
}

void
DeviceBlackboard::ProcessSimulation()
{
  if (!is_simulator())
    return;

  ScopeLock protect(mutexBlackboard);

  SetBasic().gps.Simulator = true;
  SetBasic().gps.MovementDetected = false;

#ifdef ANDROID
  SetBasic().gps.AndroidInternalGPS = false;
#endif

  SetNAVWarning(false);
  SetBasic().Location = FindLatitudeLongitude(Basic().Location,
                                              Basic().TrackBearing,
                                              Basic().GroundSpeed);
  SetBasic().Time += fixed_one;
  long tsec = (long)Basic().Time;
  SetBasic().DateTime.hour = tsec / 3600;
  SetBasic().DateTime.minute = (tsec - Basic().DateTime.hour * 3600) / 60;
  SetBasic().DateTime.second = tsec-Basic().DateTime.hour * 3600
    - Basic().DateTime.minute * 60;

  // use this to test FLARM parsing/display
  if (is_debug() && !is_altair())
    DeviceList[0].parser.TestRoutine(&SetBasic());
}

/**
 * Sets the GPS speed and indicated airspeed to val
 *
 * not in use
 * @param val New speed
 */
void
DeviceBlackboard::SetSpeed(fixed val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().GroundSpeed = val;
  SetBasic().IndicatedAirspeed = val;
}

/**
 * Sets the TrackBearing to val
 *
 * not in use
 * @param val New TrackBearing
 */
void
DeviceBlackboard::SetTrackBearing(Angle val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().TrackBearing = val.as_bearing();
}

/**
 * Sets the altitude and barometric altitude to val
 *
 * not in use
 * @param val New altitude
 */
void
DeviceBlackboard::SetAltitude(fixed val)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().GPSAltitude = val;
  SetBasic().BaroAltitude = val;
}

/**
 * Reads the given derived_info usually provided by the
 * GlideComputerBlackboard and saves it to the own Blackboard
 * @param derived_info Calculated information usually provided
 * by the GlideComputerBlackboard
 */
void
DeviceBlackboard::ReadBlackboard(const DERIVED_INFO &derived_info)
{
  calculated_info = derived_info;
}

/**
 * Reads the given settings usually provided by the InterfaceBlackboard
 * and saves it to the own Blackboard
 * @param settings SettingsComputer usually provided by the
 * InterfaceBlackboard
 */
void
DeviceBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER
					      &settings)
{
  settings_computer = settings;
}

/**
 * Reads the given settings usually provided by the InterfaceBlackboard
 * and saves it to the own Blackboard
 * @param settings SettingsMap usually provided by the
 * InterfaceBlackboard
 */
void
DeviceBlackboard::ReadSettingsMap(const SETTINGS_MAP
				  &settings)
{
  settings_map = settings;
}

/**
 * Checks for timeout of the FLARM targets and
 * saves the status to Basic()
 */
void
DeviceBlackboard::FLARM_RefreshSlots() {
  FLARM_STATE &flarm_state = SetBasic().flarm;
  flarm_state.Refresh(Basic().Time);
}

/**
 * Sets the system time to GPS time if not yet done and
 * defined in settings
 */
void
DeviceBlackboard::SetSystemTime() {
  // TODO JMW: this should be done outside the parser..
  if (is_simulator())
    return;

#ifdef WIN32
  // Altair doesn't have a battery-backed up realtime clock,
  // so as soon as we get a fix for the first time, set the
  // system clock to the GPS time.
  static bool sysTimeInitialised = false;

  const GPS_STATE &gps = Basic().gps;

  if (!gps.NAVWarning && SettingsMap().SetSystemTimeFromGPS
      && !sysTimeInitialised) {
    SYSTEMTIME sysTime;
    ::GetSystemTime(&sysTime);

    sysTime.wYear = (unsigned short)Basic().DateTime.year;
    sysTime.wMonth = (unsigned short)Basic().DateTime.month;
    sysTime.wDay = (unsigned short)Basic().DateTime.day;
    sysTime.wHour = (unsigned short)Basic().DateTime.hour;
    sysTime.wMinute = (unsigned short)Basic().DateTime.minute;
    sysTime.wSecond = (unsigned short)Basic().DateTime.second;
    sysTime.wMilliseconds = 0;
    sysTimeInitialised = (::SetSystemTime(&sysTime)==true);

#if defined(_WIN32_WCE) && defined(GNAV)
    TIME_ZONE_INFORMATION tzi;
    tzi.Bias = -SettingsComputer().UTCOffset/60;
    _tcscpy(tzi.StandardName,TEXT("Altair"));
    tzi.StandardDate.wMonth= 0; // disable daylight savings
    tzi.StandardBias = 0;
    _tcscpy(tzi.DaylightName,TEXT("Altair"));
    tzi.DaylightDate.wMonth= 0; // disable daylight savings
    tzi.DaylightBias = 0;

    SetTimeZoneInformation(&tzi);
#endif
    sysTimeInitialised =true;
  }
#else
  // XXX
#endif
}

/**
 * Tries to find a name for every current FLARM_Traffic id
 */
void
DeviceBlackboard::FLARM_ScanTraffic()
{
  // TODO: this is a bit silly, it searches every time a target is
  // visible... going to be slow..
  // should only scan the first time it appears with that ID.
  // at least it is now not being done by the parser

  FLARM_STATE &flarm = SetBasic().flarm;

  // if (FLARM data is available)
  if (!flarm.FLARM_Available)
    return;

  // for each item in FLARM_Traffic
  for (unsigned i = 0; i < FLARM_STATE::FLARM_MAX_TRAFFIC; i++) {
    FLARM_TRAFFIC &traffic = flarm.FLARM_Traffic[i];

    // if (FLARM_Traffic[flarm_slot] has data)
    // and if (Target currently without name)
    if (traffic.defined() && !traffic.HasName()) {
      // need to lookup name for this target
      const TCHAR *fname = FlarmDetails::LookupCallsign(traffic.ID);
      if (fname != NULL)
        _tcscpy(traffic.Name, fname);
    }
  }
}

void
DeviceBlackboard::tick(const GlidePolar& glide_polar)
{
  // check for timeout on FLARM objects
  FLARM_RefreshSlots();
  // lookup known traffic
  FLARM_ScanTraffic();
  // set system time if necessary
  SetSystemTime();

  // calculate fast data to complete aircraft state
  FlightState(glide_polar);
  Wind();
  Heading();
  NavAltitude();
  AutoQNH();

  tick_fast(glide_polar);

  TurnRate();
  if (Basic().Time!= LastBasic().Time) {

    if (Basic().Time > LastBasic().Time) {
      Dynamics();
    }

    state_last = Basic();
  }
}


void
DeviceBlackboard::tick_fast(const GlidePolar& glide_polar)
{
  EnergyHeight();
  WorkingBand();
  Vario();
  NettoVario(glide_polar);
}


void
DeviceBlackboard::NettoVario(const GlidePolar& glide_polar)
{
  SetBasic().GliderSinkRate = Basic().flight.Flying
    ? - glide_polar.SinkRate(Basic().IndicatedAirspeed,
                             Basic().acceleration.Gload)
    /* the glider sink rate is useless when not flying */
    : fixed_zero;

  if (!Basic().NettoVarioAvailable)
    SetBasic().NettoVario = Basic().TotalEnergyVario
      - Basic().GliderSinkRate;
}



/**
 * 1. Determines which altitude to use (GPS/baro)
 * 2. Calculates height over ground
 */
void
DeviceBlackboard::NavAltitude()
{
  if (!SettingsComputer().EnableNavBaroAltitude
      || !Basic().BaroAltitudeAvailable) {
    SetBasic().NavAltitude = Basic().GPSAltitude;
  } else {
    SetBasic().NavAltitude = Basic().BaroAltitude;
  }
  SetBasic().AltitudeAGL = Basic().NavAltitude
    - Calculated().TerrainAlt;
}


/**
 * Calculates the heading, and the estimated true airspeed
 */
void
DeviceBlackboard::Heading()
{
  const SpeedVector wind = Basic().wind;

  if (positive(Basic().GroundSpeed) || wind.is_non_zero()) {
    fixed x0 = (Basic().TrackBearing.fastsine()) * Basic().GroundSpeed;
    fixed y0 = (Basic().TrackBearing.fastcosine())
      * Basic().GroundSpeed;
    x0 += (wind.bearing.fastsine()) * wind.norm;
    y0 += (wind.bearing.fastcosine()) * wind.norm;

    if (!Basic().flight.Flying) {
      // don't take wind into account when on ground
      SetBasic().Heading = Basic().TrackBearing;
    } else {
      SetBasic().Heading = Angle::radians(atan2(x0, y0)).as_bearing();
    }

    // calculate estimated true airspeed
    SetBasic().TrueAirspeedEstimated = hypot(x0, y0);

  } else {
    SetBasic().Heading = Basic().TrackBearing;
    SetBasic().TrueAirspeedEstimated = fixed_zero;
  }

  if (!Basic().AirspeedAvailable) {
    SetBasic().TrueAirspeed = Basic().TrueAirspeedEstimated;
    SetBasic().IndicatedAirspeed = Basic().TrueAirspeed
      / AtmosphericPressure::AirDensityRatio(Basic().GetAltitudeBaroPreferred());
  }
}

/**
 * 1. Calculates the vario values for gps vario, gps total energy vario and distance vario
 * 2. Sets Vario to GPSVario or received Vario data from instrument
 */
void
DeviceBlackboard::Vario()
{
  // Calculate time passed since last calculation
  const fixed dT = Basic().Time - LastBasic().Time;

  if (positive(dT)) {
    const fixed Gain = Basic().NavAltitude - LastBasic().NavAltitude;
    const fixed GainTE = Basic().TEAltitude - LastBasic().TEAltitude;

    // estimate value from GPS
    SetBasic().GPSVario = Gain / dT;
    SetBasic().GPSVarioTE = GainTE / dT;
  }

  if (!Basic().TotalEnergyVarioAvailable)
    SetBasic().TotalEnergyVario = Basic().GPSVario;
}


void
DeviceBlackboard::Wind()
{
  if (Basic().ExternalWindAvailable && SettingsComputer().ExternalWind)
    SetBasic().wind = Basic().ExternalWind;
  else if (SettingsComputer().AutoWindMode == 0)
    SetBasic().wind = SettingsComputer().ManualWind;
  else
    SetBasic().wind = Calculated().estimated_wind;
}

/**
 * Calculates the turn rate
 */
void
DeviceBlackboard::TurnRate()
{
  // Calculate time passed since last calculation
  const fixed dT = Basic().Time - LastBasic().Time;

  // Calculate turn rate

  if (!Basic().flight.Flying) {
    SetBasic().TurnRate = fixed_zero;
    return;
  }
  if (!positive(dT)) {
    return;
  }

  SetBasic().TurnRate =
    (Basic().TrackBearing - LastBasic().TrackBearing).as_delta().value_degrees() / dT;
}

/**
 * Calculates the turn rate of the heading,
 * the estimated bank angle and
 * the estimated pitch angle
 */
void
DeviceBlackboard::Dynamics()
{
  if (Basic().flight.Flying &&
      (positive(Basic().GroundSpeed) ||
       Basic().wind.is_non_zero())) {

    // calculate turn rate in wind coordinates
    const fixed dT = Basic().Time - LastBasic().Time;

    if (positive(dT)) {
      SetBasic().TurnRateWind =
        (Basic().Heading - LastBasic().Heading).as_delta().value_degrees() / dT;
    }

    // estimate bank angle (assuming balanced turn)
    const fixed angle = atan(Angle::degrees(Basic().TurnRateWind
        * Basic().TrueAirspeed * fixed_inv_g).value_radians());

    SetBasic().acceleration.BankAngle = Angle::radians(angle);

    if (!Basic().acceleration.Available)
      SetBasic().acceleration.Gload = fixed_one
        / max(fixed_small, fabs(cos(angle)));

    // estimate pitch angle (assuming balanced turn)
    SetBasic().acceleration.PitchAngle = Angle::radians(
      atan2(Basic().GPSVario - Basic().TotalEnergyVario,
            Basic().TrueAirspeed));

  } else {
    SetBasic().acceleration.BankAngle = Angle::native(fixed_zero);
    SetBasic().acceleration.PitchAngle = Angle::native(fixed_zero);
    SetBasic().TurnRateWind = fixed_zero;

    if (!Basic().acceleration.Available)
      SetBasic().acceleration.Gload = fixed_one;
  }
}


/**
 * Calculates energy height on TAS basis
 *
 * \f${m/2} \times v^2 = m \times g \times h\f$ therefore \f$h = {v^2}/{2 \times g}\f$
 */
void
DeviceBlackboard::EnergyHeight()
{
  SetBasic().EnergyHeight =
      Basic().TrueAirspeed * Basic().TrueAirspeed * fixed_inv_2g;
  SetBasic().TEAltitude = Basic().NavAltitude + Basic().EnergyHeight;
}

void
DeviceBlackboard::WorkingBand()
{
  const fixed working_band_height = Basic().TEAltitude -
                                    SettingsComputer().safety_height_terrain -
                                    Calculated().TerrainBase;
 
  SetBasic().working_band_height = working_band_height;
    if (negative(Basic().working_band_height)) {
    SetBasic().working_band_fraction = fixed_zero;
    return;
  }

  const fixed max_height = Calculated().thermal_band.MaxThermalHeight;
  if (positive(max_height))
    SetBasic().working_band_fraction = working_band_height / max_height;
  else
    SetBasic().working_band_fraction = fixed_one;
}

void
DeviceBlackboard::FlightState(const GlidePolar& glide_polar)
{
  if (Basic().Time < LastBasic().Time)
    SetBasic().flight.flying_state_reset();

  // GPS not lost
  if (Basic().gps.NAVWarning)
    return;

  // Speed too high for being on the ground
  const fixed speed = Basic().AirspeedAvailable?
    std::max(SetBasic().TrueAirspeed, Basic().GroundSpeed): Basic().GroundSpeed;

  if (speed > glide_polar.get_Vtakeoff() ||
      (Calculated().TerrainValid && Basic().AltitudeAGL > fixed(300))) {
    SetBasic().flight.flying_state_moving(Basic().Time);
  } else {
    SetBasic().flight.flying_state_stationary(Basic().Time);
  }
}

void
DeviceBlackboard::AutoQNH()
{
  #define QNH_TIME 10

  static unsigned countdown_autoqnh = QNH_TIME;

  if (!Basic().flight.OnGround // must be on ground
      || !countdown_autoqnh    // only do it once
      || Basic().gps.Replay // never in replay mode
      || Basic().gps.Simulator // never in simulator
      || Basic().gps.NAVWarning // Reject if no valid GPS fix
      || !Basic().BaroAltitudeAvailable // Reject if no baro altitude
    ) {
    if (countdown_autoqnh<= QNH_TIME) {
      countdown_autoqnh= QNH_TIME; // restart if havent performed
    }
    return;
  }

  if (countdown_autoqnh<= QNH_TIME)
    countdown_autoqnh--;

  if (!countdown_autoqnh) {
    SetBasic().pressure.FindQNH(Basic().BaroAltitude, 
                                Calculated().TerrainAlt);
    AllDevicesPutQNH(Basic().pressure);
    countdown_autoqnh = UINT_MAX; // disable after performing once
  }
}

void
DeviceBlackboard::SetQNH(fixed qnh)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().pressure.set_QNH(qnh);
  AllDevicesPutQNH(Basic().pressure);
}

void
DeviceBlackboard::SetMC(fixed mc)
{
  ScopeLock protect(mutexBlackboard);
  SetBasic().MacCready = mc;
  AllDevicesPutMacCready(mc);
  TriggerGPSUpdate();
}
