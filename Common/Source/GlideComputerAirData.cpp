/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include "GlideComputerAirData.hpp"
#include "WindZigZag.h"
#include "WindAnalyser.h"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Math/LowPassFilter.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/FastMath.h"
#include "Math/Constants.h"
#include "RasterTerrain.h"
#include "RasterMap.h"
#include "Calibration.hpp"
#include "GlideTerrain.hpp"
#include "SettingsTask.hpp"
#include "LocalTime.hpp"
#include "MapWindowProjection.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Atmosphere.h"
#include "LogFile.hpp"
#include "GPSClock.hpp"

#include "GlideSolvers/GlidePolar.hpp"

#define TAKEOFFSPEEDTHRESHOLD (0.5*glide_polar.get_Vmin())

#ifndef _MSC_VER
#include <algorithm>
using std::min;
using std::max;
#endif

void
DoAutoQNH(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated, double speed_threshold);

#define MinTurnRate  4
#define CruiseClimbSwitch 15
#define ClimbCruiseSwitch 10
#define THERMAL_TIME_MIN 45.0


GlideComputerAirData::GlideComputerAirData(AirspaceWarningManager& as_manager,
  const GlidePolar& _glide_polar):
  m_airspace_warning(as_manager),
  airspace_clock(2.0), // scan airspace every 2 seconds
  ballast_clock(5),  // only update every 5 seconds to stop flooding
		    // the devices
  diff_gps_vario(0),
  diff_gps_te_vario(0),
  glide_polar(_glide_polar)
{
  InitLDRotary(SettingsComputer(), &rotaryLD);

  // JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(Calculated().WindSpeed, Calculated().WindBearing, 1);
}

void
GlideComputerAirData::ResetFlight(const bool full)
{
  m_airspace_warning.reset(Basic());

  diff_gps_vario.reset(0,0);
  diff_gps_te_vario.reset(0,0);
}

void
GlideComputerAirData::Initialise()
{
  CalibrationInit();
}


/**
 * Calculates some basic values
 */
void
GlideComputerAirData::ProcessBasic()
{
  EnergyHeight();
  TerrainHeight();
  Vario();
  ProcessSun();
  SetCalculated().AdjustedAverageThermal = GetAverageThermal();
}

/**
 * Calculates some other values
 */
void
GlideComputerAirData::ProcessVertical()
{
  Turning();
  Wind();
  ProcessThermalLocator();
  CuSonde::updateMeasurements(Basic(), Calculated());
  LastThermalStats();
  LD();
  CruiseLD();
  SetCalculated().AverageLD = CalculateLDRotary(Calculated(), rotaryLD);
  Average30s();
  AverageClimbRate();
  AverageThermal();
  ThermalGain();
}

/**
 * Calculates the wind
 */
void
GlideComputerAirData::Wind()
{

  if (!Calculated().Flying || !time_advanced())
    return;

  if (Calculated().TurnMode == CLIMB)
    DoWindCirclingSample();

  // generate new wind vector if altitude changes or a new
  // estimate is available
  DoWindCirclingAltitude();

  // update zigzag wind
  if (((SettingsComputer().AutoWindMode & D_AUTOWIND_ZIGZAG) == D_AUTOWIND_ZIGZAG)
      && (!Basic().Replay)) {
    double zz_wind_speed;
    double zz_wind_bearing;
    int quality;
    quality = WindZigZagUpdate(&Basic(), &Calculated(),
			       &zz_wind_speed,
			       &zz_wind_bearing);

    if (quality > 0) {
      SetWindEstimate(zz_wind_speed, zz_wind_bearing, quality);
      Vector v_wind;
      v_wind.x = zz_wind_speed*cos(zz_wind_bearing*DEG_TO_RAD);
      v_wind.y = zz_wind_speed*sin(zz_wind_bearing*DEG_TO_RAD);

      windanalyser.slot_newEstimate(&Basic(), &SetCalculated(), v_wind, quality);
    }
  }
}

/**
 * Passes data to the windanalyser.slot_newFlightMode method
 */
void
GlideComputerAirData::DoWindCirclingMode(const bool left)
{
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING) == D_AUTOWIND_CIRCLING)
    windanalyser.slot_newFlightMode(&Basic(), &Calculated(), left, 0);
}

/**
 * Passes data to the windanalyser.slot_newSample method
 */
void
GlideComputerAirData::DoWindCirclingSample()
{
  if ((SettingsComputer().AutoWindMode & D_AUTOWIND_CIRCLING) == D_AUTOWIND_CIRCLING)
    windanalyser.slot_newSample(&Basic(), &SetCalculated());
}

/**
 * Passes data to the windanalyser.SlotAltitude method
 */
void
GlideComputerAirData::DoWindCirclingAltitude()
{
  if (SettingsComputer().AutoWindMode > 0)
    windanalyser.slot_Altitude(&Basic(), &SetCalculated());
}

void
GlideComputerAirData::SetWindEstimate(const double wind_speed,
    const double wind_bearing, const int quality)
{
  Vector v_wind;
  v_wind.x = wind_speed*cos(wind_bearing*M_PI/180.0);
  v_wind.y = wind_speed*sin(wind_bearing*M_PI/180.0);

  windanalyser.slot_newEstimate(&Basic(), &SetCalculated(), v_wind, quality);
}

void
GlideComputerAirData::AverageClimbRate()
{
  if (Basic().AirspeedAvailable && Basic().VarioAvailable
      && (!Calculated().Circling)) {

    int vi = iround(Basic().IndicatedAirspeed);

    if ((vi <= 0) || (vi >= SettingsComputer().SafetySpeed)) {
      // out of range
      return;
    }

    if (Basic().AccelerationAvailable) {
      if (fabs(fabs(Basic().Gload) - 1.0) > 0.25) {
        // G factor too high
        return;
      }
    }

    if (Basic().TrueAirspeed>0) {
      // TODO: Check this is correct for TAS/IAS
      double ias_to_tas = Basic().IndicatedAirspeed / Basic().TrueAirspeed;
      double w_tas = Basic().Vario * ias_to_tas;

      SetCalculated().AverageClimbRate[vi] += w_tas;
      SetCalculated().AverageClimbRateN[vi]++;
    }
  }
}

#ifdef NEWCLIMBAV
ClimbAverageCalculator climbAverageCalculator;
void
GlideComputerAirData::Average30s()
{
  Calculated().Average30s =
    climbAverageCalculator.GetAverage(Basic().Time, Basic().Altitude, 30);
  Calculated().NettoAverage30s = Calculated().Average30s;
}
#else

void
GlideComputerAirData::Average30s()
{
  static double LastTime = 0;
  static double Altitude[30];
  static double Vario[30];
  static double NettoVario[30];
  int Elapsed, i;
  long index = 0;
  double Gain;
  static int num_samples = 0;

  if (time_advanced()) {
    if (Calculated().Circling != LastCalculated().Circling)
      num_samples = 0;
      // reset!


    Elapsed = (int)(Basic().Time - LastBasic().Time);
    for (i = 0; i < Elapsed; i++) {
      index = (long)LastTime + i;
      index %= 30;

      Altitude[index] = Basic().NavAltitude;

      if (Basic().NettoVarioAvailable) {
        NettoVario[index] = Basic().NettoVario;
      } else {
        NettoVario[index] = Calculated().NettoVario;
      }

      if (Basic().VarioAvailable) {
        Vario[index] = Basic().Vario;
      } else {
        Vario[index] = Calculated().Vario;
      }

      if (num_samples < 30) {
        num_samples++;
      }
    }

    double Vave = 0;
    double NVave = 0;
    int j;

    for (i = 0; i < num_samples; i++) {
      j = (index - i) % 30;
      if (j < 0) {
        j += 30;
      }
      Vave += Vario[j];
      NVave += NettoVario[j];
    }

    if (num_samples) {
      Vave /= num_samples;
      NVave /= num_samples;
    }

    if (!Basic().VarioAvailable) {
      index = (Basic().Time.as_long() - 1)%30;
      Gain = Altitude[index];

      index = Basic().Time.as_long()%30;
      Gain = Gain - Altitude[index];

      Vave = Gain / 30;
    }
    SetCalculated().Average30s =
      LowPassFilter(Calculated().Average30s, Vave, 0.8);
    SetCalculated().NettoAverage30s =
      LowPassFilter(Calculated().NettoAverage30s, NVave, 0.8);

#ifdef DEBUGAVERAGER
    if (Calculated().Flying) {
      DebugStore("%d %g %g %g # averager\r\n",
		 num_samples,
		 Calculated().Vario,
		 Calculated().Average30s, Calculated().NettoAverage30s);
    }
#endif

  } else {
    if (time_retreated()) {
      for (i = 0; i < 30; i++) {
        Altitude[i] = 0;
        Vario[i] = 0;
        NettoVario[i] = 0;
      }
    }
  }
}
#endif


void
GlideComputerAirData::AverageThermal()
{
  if (Calculated().ClimbStartTime >= 0) {
    if (Basic().Time > Calculated().ClimbStartTime) {
      double Gain = Basic().NavAltitude + Calculated().EnergyHeight
          - Calculated().ClimbStartAlt;
      SetCalculated().AverageThermal =
          Gain / (Basic().Time - Calculated().ClimbStartTime);
      }
  }
}

void
GlideComputerAirData::MaxHeightGain()
{
  if (!Calculated().Flying)
    return;

  if (Calculated().MinAltitude > 0) {
    double height_gain = Basic().NavAltitude - Calculated().MinAltitude;
    SetCalculated().MaxHeightGain = max(height_gain, Calculated().MaxHeightGain);
  } else {
    SetCalculated().MinAltitude = Basic().NavAltitude;
  }

  SetCalculated().MinAltitude = min(Basic().NavAltitude, Calculated().MinAltitude);
}

void
GlideComputerAirData::ThermalGain()
{
  if (Calculated().ClimbStartTime >= 0) {
    if(Basic().Time >= Calculated().ClimbStartTime) {
      SetCalculated().ThermalGain = Basic().NavAltitude
          + Calculated().EnergyHeight - Calculated().ClimbStartAlt;
    }
  }
}

void
GlideComputerAirData::LD()
{
  if (time_retreated()) {
    SetCalculated().LDvario = INVALID_GR;
    SetCalculated().LD = INVALID_GR;
  }

  if (time_advanced()) {
    double DistanceFlown = Distance(Basic().Location, LastBasic().Location);

    SetCalculated().LD =
      UpdateLD(Calculated().LD,
	       DistanceFlown,
	       Basic().NavAltitude - LastBasic().NavAltitude, 0.1);

    InsertLDRotary(Calculated(),
		   &rotaryLD,(int)DistanceFlown,
		   (int)Basic().NavAltitude);
  }

  // LD instantaneous from vario, updated every reading..
  if (Basic().VarioAvailable && Basic().AirspeedAvailable && Calculated().Flying) {
    SetCalculated().LDvario = UpdateLD(Calculated().LDvario,
				       Basic().IndicatedAirspeed,
				       -Basic().Vario,
				       0.3);
  } else {
    SetCalculated().LDvario = INVALID_GR;
  }
}

void
GlideComputerAirData::CruiseLD()
{
  if(!Calculated().Circling) {
    if (Calculated().CruiseStartTime < 0) {
      SetCalculated().CruiseStartLocation = Basic().Location;
      SetCalculated().CruiseStartAlt = Basic().NavAltitude;
      SetCalculated().CruiseStartTime = Basic().Time;
    } else {
      double DistanceFlown = Distance(Basic().Location,
                                      Calculated().CruiseStartLocation);
      SetCalculated().CruiseLD =
          UpdateLD(Calculated().CruiseLD,
                   DistanceFlown,
                   Calculated().CruiseStartAlt - Basic().NavAltitude,
                   0.5);
    }
  }
}


/**
 * 1. Determines which altitude to use (GPS/baro)
 * 2. If possible calculates true airspeed and TAS/IAS ratio
 * 3. Calculates energy height on TAS basis
 *
 * \f${m/2} \times v^2 = m \times g \times h\f$ therefore \f$h = {v^2}/{2 \times g}\f$
 */
void
GlideComputerAirData::EnergyHeight()
{
  double ias_to_tas;
  double V_tas;

  // Calculate true airspeed
  if (Basic().AirspeedAvailable && (Basic().IndicatedAirspeed > 0)) {
    ias_to_tas = Basic().TrueAirspeed / Basic().IndicatedAirspeed;
  } else {
    ias_to_tas = 1.0;
  }

  V_tas = Basic().TrueAirspeed;

  // Calculate energy height
  double V_bestld_tas = glide_polar.get_VbestLD() * ias_to_tas;
  double V_mc_tas = Calculated().VMacCready * ias_to_tas;
  V_tas = max(V_tas, V_bestld_tas);

  double V_target = max(V_bestld_tas, V_mc_tas);
  SetCalculated().EnergyHeight = (V_tas * V_tas - V_target * V_target) / (9.81 * 2.0);
}

/**
 * Reads the current terrain height and calculates the altitude over terrain
 */
void
GlideComputerAirData::TerrainHeight()
{
  short Alt = -1;

  terrain.Lock();
  if (terrain.GetMap()) {
    // want most accurate rounding here
    RasterRounding rounding(*terrain.GetMap(),0,0);
    Alt = terrain.GetTerrainHeight(Basic().Location, rounding);
  }
  terrain.Unlock();

  if (Alt < 0) {
    Alt = 0;
    // QUESTION TB: this can't work right?!
    if (Alt <= TERRAIN_INVALID) {
      SetCalculated().TerrainValid = false;
    } else {
      SetCalculated().TerrainValid = true;
    }
    SetCalculated().TerrainAlt = 0;
  } else {
    SetCalculated().TerrainValid = true;
    SetCalculated().TerrainAlt = Alt;
  }

  SetCalculated().AltitudeAGL = Basic().NavAltitude - Calculated().TerrainAlt;

  if (!SettingsComputer().FinalGlideTerrain) {
    SetCalculated().TerrainBase = Calculated().TerrainAlt;
  }
}

/**
 * 1. Calculates the vario values for gps vario, gps total energy vario and distance vario
 * 2. Sets Vario to GPSVario or received Vario data from instrument
 */
void
GlideComputerAirData::Vario()
{
  double dT = Basic().Time - LastBasic().Time;
  if (dT > 0) {
    double Gain = Basic().NavAltitude - LastBasic().NavAltitude;
    double GainTE = (Calculated().EnergyHeight + Basic().GPSAltitude)
        - (LastCalculated().EnergyHeight + LastBasic().GPSAltitude);

    // estimate value from GPS
    SetCalculated().GPSVario = Gain / dT;
    SetCalculated().GPSVarioTE = GainTE / dT;
  }

  if (!Basic().VarioAvailable || Basic().Replay) {
    // TODO: why not TE?!
    SetCalculated().Vario = Calculated().GPSVario;
  } else {
    // get value from instrument
    SetCalculated().Vario = Basic().Vario;
    // we don't bother with sound here as it is polled at a
    // faster rate in the DoVarioCalcs methods
  }

  if (Basic().VarioAvailable && !Basic().Replay) {
    CalibrationUpdate(&Basic(), &Calculated());
  }
}

void
GlideComputerAirData::SpeedToFly()
{
  const double n = fabs(Basic().Gload);

  // calculate optimum cruise speed in current track direction
  // this still makes use of mode, so it should agree with
  // Vmaccready if the track bearing is the best cruise track
  // this does assume g loading of 1.0

  // this is basically a dolphin soaring calculator

#ifdef OLD_TASK
  double delta_mc;
  double risk_mc;

  double margin_height = Basic().NavAltitude
    + Calculated().EnergyHeight - SettingsComputer().SafetyAltitudeBreakoff
    - Calculated().TerrainBase;

  if ((Calculated().TaskAltitudeDifference > -120) || (Calculated().MaxThermalHeight<1)) {
    risk_mc = mc_setting;
  } else {

    risk_mc =
      glide_polar.mc_risk(margin_height/Calculated().MaxThermalHeight, SettingsComputer().RiskGamma);
  }
  SetCalculated().MacCreadyRisk = risk_mc;
#endif
}

void
GlideComputerAirData::NettoVario()
{
  const double n = fabs(Basic().Gload);

  // calculate sink rate of glider for calculating netto vario

  bool replay_disabled = !Basic().Replay;

  double glider_sink_rate;
  fixed speed = (Basic().AirspeedAvailable && replay_disabled)? Basic().IndicatedAirspeed: Basic().TrueAirspeed;
  glider_sink_rate= -glide_polar.SinkRate(max(glide_polar.get_Vmin(), speed), n);
  SetCalculated().GliderSinkRate = glider_sink_rate;

  if (Basic().NettoVarioAvailable && replay_disabled) {
    SetCalculated().NettoVario = Basic().NettoVario;
  } else {
    if (Basic().VarioAvailable && replay_disabled) {
      SetCalculated().NettoVario = Basic().Vario - glider_sink_rate;
    } else {
      SetCalculated().NettoVario = Calculated().Vario - glider_sink_rate;
    }
  }
}

bool
GlideComputerAirData::ProcessVario()
{
  NettoVario();
  SpeedToFly();

  // has GPS time advanced?
  return time_advanced();
}

/**
 * 1. Detects time retreat and calls ResetFlight if GPS lost
 * 2. Detects change in replay status and calls ResetFlight if so
 * 3. Calls DetectStartTime and saves the time of flight
 * @return true as default, false if something is wrong in time
 */
bool
GlideComputerAirData::FlightTimes()
{
  if ((Basic().Time != 0) && time_retreated()) {
    // 20060519:sgi added (Basic().Time != 0) due to always return here
    // if no GPS time available
    if (!Basic().NAVWarning) {
      // Reset statistics.. (probably due to being in IGC replay mode)
      ResetFlight(false);
    }
    return false;
  }

  if (Basic().Replay != LastBasic().Replay) {
    // reset flight before/after replay logger
    ResetFlight(false);
  }

  double t = DetectStartTime(&Basic(), &Calculated());
  if (t>0) {
    SetCalculated().FlightTime = t;
  }

  TakeoffLanding();

  return true;
}

void
GlideComputerAirData::ProcessIdle()
{
  BallastDump();
  TerrainFootprint(MapProjection().GetScreenDistanceMeters());
  if (airspace_clock.check_advance(Basic().Time)
      && SettingsComputer().EnableAirspaceWarnings) {
    AirspaceWarning();
  }
}

/**
 * Detects takeoff and landing events
 */
void
GlideComputerAirData::TakeoffLanding()
{
  if (time_retreated()) {
    SetCalculated().TimeInFlight=0;
    SetCalculated().TimeOnGround=0;
  }

  if (Basic().Speed>1.0) {
    // stop system from shutting down if moving
    XCSoarInterface::InterfaceTimeoutReset();
  }

  // GPS not lost
  if (!Basic().NAVWarning) {
    // Speed too high for being on the ground
    if (Basic().Speed> TAKEOFFSPEEDTHRESHOLD) {
      SetCalculated().TimeInFlight= LastCalculated().TimeInFlight+1;
      SetCalculated().TimeOnGround= 0;
    } else {
      if ((Calculated().AltitudeAGL < 300) && (Calculated().TerrainValid)) {
        SetCalculated().TimeInFlight = LastCalculated().TimeInFlight - 1;
      } else if (!Calculated().TerrainValid) {
        SetCalculated().TimeInFlight = LastCalculated().TimeInFlight - 1;
      }
      SetCalculated().TimeOnGround= LastCalculated().TimeOnGround+1;
    }
  }

  SetCalculated().TimeOnGround = min(30,max(0,Calculated().TimeOnGround));
  SetCalculated().TimeInFlight = min(60,max(0,Calculated().TimeInFlight));

  // JMW logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds
  //
  // TODO accuracy: make this more robust by making use of terrain height data
  // if available

  if ((Calculated().TimeOnGround <= 10) || (Basic().Replay)) {
    // Don't allow 'OnGround' calculations if in IGC replay mode
    SetCalculated().OnGround = false;
  }

  if (!Calculated().Flying) {
    // detect takeoff
    if (Calculated().TimeInFlight > 10)
      OnTakeoff();

    if (Calculated().TimeOnGround > 10) {
      SetCalculated().OnGround = true;
      DoAutoQNH(&Basic(), &Calculated(), TAKEOFFSPEEDTHRESHOLD);
    }
  } else {
    // detect landing
    if (Calculated().TimeInFlight == 0) {
      // have been stationary for a minute
      OnLanding();
    }
  }
}

void
GlideComputerAirData::OnLanding()
{
  // JMWX  restore data calculated at finish so
  // user can review flight as at finish line

#ifdef OLD_TASK
  if (Calculated().ValidFinish) {
    RestoreFinish();
  }
#endif
  SetCalculated().Flying = false;
}

void
GlideComputerAirData::OnTakeoff()
{
  SetCalculated().Flying = true;

  // reset stats on takeoff
  ResetFlight();

  SetCalculated().TakeOffTime = Basic().Time;

  // save stats in case we never finish
  SaveFinish();
}



void
GlideComputerAirData::AirspaceWarning()
{
  // JMW OLD_TASK this locking is just for now since we don't have any protection
  terrain.Lock();
  if (m_airspace_warning.update(Basic())) {
    airspaceWarningEvent.trigger();
  }
  terrain.Unlock();
}


void
GlideComputerAirData::TerrainFootprint(double screen_range)
{
  if (!SettingsComputer().FinalGlideTerrain)
    return;

  double bearing, distance;
  bool out_of_range;

  // estimate max range (only interested in at most one screen
  // distance away) except we need to scan for terrain base, so 20km
  // search minimum is required

  double mymaxrange = max(20000.0, screen_range);

  SetCalculated().TerrainBase = Calculated().TerrainAlt;

  GEOPOINT loc;
  for (int i = 0; i <= NUMTERRAINSWEEPS; i++) {
    bearing = (i * 360.0) / NUMTERRAINSWEEPS;

    terrain.Lock();
    distance = FinalGlideThroughTerrain(bearing, Basic(), Calculated(),
        SettingsComputer(), terrain, &loc, mymaxrange, &out_of_range,
        &SetCalculated().TerrainBase);
    terrain.Unlock();

    if (out_of_range) {
      FindLatitudeLongitude(Basic().Location, bearing, mymaxrange * 20, &loc);
    }

    SetCalculated().GlideFootPrint[i].Longitude = loc.Longitude;
    SetCalculated().GlideFootPrint[i].Latitude = loc.Latitude;
  }

  SetCalculated().Experimental = Calculated().TerrainBase;
}

void
GlideComputerAirData::BallastDump()
{
  double dt = ballast_clock.delta_advance(Basic().Time);

  if (!SettingsComputer().BallastTimerActive || (dt <= 0)) {
    return;
  }

  double BALLAST = glide_polar.get_ballast();
  double BALLAST_last = BALLAST;
  double percent_per_second =
      1.0 / max(10.0, (double)SettingsComputer().BallastSecsToEmpty);

  BALLAST -= dt * percent_per_second;
  if (BALLAST < 0) {
    /// TODO SettingsComputer().BallastTimerActive = false;
    BALLAST = 0.0;
  }
#ifdef OLD_TASK
  glide_polar.set_ballast(BALLAST);
  task.set_glide_polar(glide_polar);
#endif
}

void
GlideComputerAirData::OnSwitchClimbMode(bool isclimb, bool left)
{
  InitLDRotary(SettingsComputer(), &rotaryLD);

  // Tell the windanalyser of the new flight mode
  DoWindCirclingMode(left);
}

/**
 * Calculate the circling time percentage and call the thermal band calculation
 * @param Rate Current turn rate
 */
void
GlideComputerAirData::PercentCircling(const double Rate)
{
  // TODO accuracy: TB: this would only work right if called every ONE second!

  // JMW circling % only when really circling,
  // to prevent bad stats due to flap switches and dolphin soaring

  // if (Circling)
  if (Calculated().Circling && (Rate > MinTurnRate)) {
    // Add one second to the circling time
    // timeCircling += (Basic->Time-LastTime);
    SetCalculated().timeCircling += 1.0;

    // Add the Vario signal to the total climb height
    SetCalculated().TotalHeightClimb += Calculated().GPSVario;

    // QUESTION TB: should the thermal band function really be called here
    // instead of in the Turning() function
    ThermalBand();
  } else {
    // Add one second to the cruise time
    // timeCruising += (Basic->Time-LastTime);
    SetCalculated().timeCruising += 1.0;
  }

  // Calculate the circling percentage
  if (Calculated().timeCruising + Calculated().timeCircling > 1) {
    SetCalculated().PercentCircling = 100.0 * (Calculated().timeCircling) /
        (Calculated().timeCruising + Calculated().timeCircling);
  } else {
    SetCalculated().PercentCircling = 0.0;
  }
}

void
GlideComputerAirData::ProcessThermalLocator()
{
  if (!SettingsComputer().EnableThermalLocator)
    return;

  if (Calculated().Circling) {
    thermallocator.AddPoint(Basic().Time, Basic().Location,
        Calculated().NettoVario);

    thermallocator.Update(Basic().Time, Basic().Location,
        Basic().WindSpeed, Basic().WindDirection, Basic().TrackBearing,
        &SetCalculated().ThermalEstimate_Location,
        &SetCalculated().ThermalEstimate_W, &SetCalculated().ThermalEstimate_R);
  } else {
    SetCalculated().ThermalEstimate_W = 0;
    SetCalculated().ThermalEstimate_R = -1;
    thermallocator.Reset();
  }
}

/**
 * Calculates the turn rate and the derived features.
 * Determines the current flight mode (cruise/circling).
 */
void
GlideComputerAirData::Turning()
{
  static bool LEFT = false;

  // You can't be circling unless you're flying
  if (!Calculated().Flying || !time_advanced())
    return;

  // JMW limit rate to 50 deg per second otherwise a big spike
  // will cause spurious lock on circling for a long time
  double Rate = max(-50.0, min(50.0, Basic().TurnRate));

  // average rate, to detect essing
  // TODO: use rotary buffer
  static double rate_history[60];
  double rate_ave = 0;
  for (int i = 59; i > 0; i--) {
    rate_history[i] = rate_history[i - 1];
    rate_ave += rate_history[i];
  }
  rate_history[0] = Rate;
  rate_ave /= 60;

  // Make the turn rate more smooth using the LowPassFilter
  Rate = LowPassFilter(LastCalculated().SmoothedTurnRate, Rate, 0.3);
  SetCalculated().SmoothedTurnRate = Rate;

  // Determine which direction we are circling
  if(Rate < 0) {
    // QUESTION TB: why not just LEFT = true; ?!
    if (LEFT) {
      // OK, already going left
    } else {
      LEFT = true;
    }
    Rate *= -1;
  } else {
    if (!LEFT) {
      // OK, already going right
    } else {
      LEFT = false;
    }
  }

  // Calculate circling time percentage and call thermal band calculation
  PercentCircling(Rate);

  // Force cruise or climb mode if external device says so
  bool forcecruise = false;
  bool forcecircling = false;
  if (SettingsComputer().EnableExternalTriggerCruise && !(Basic().Replay)) {
    forcecircling = triggerClimbEvent.test();
    forcecruise = !forcecircling;
  }

  switch (Calculated().TurnMode) {
  case CRUISE:
    // If (in cruise mode and beginning of circling detected)
    if ((Rate >= MinTurnRate) || (forcecircling)) {
      // Remember the start values of the turn
      SetCalculated().TurnStartTime = Basic().Time;
      SetCalculated().TurnStartLocation = Basic().Location;
      SetCalculated().TurnStartAltitude = Basic().NavAltitude;
      SetCalculated().TurnStartEnergyHeight = Calculated().EnergyHeight;
      SetCalculated().TurnMode = WAITCLIMB;
    }
    if (forcecircling) {
      // QUESTION TB: this is useless/done before ?!
      SetCalculated().TurnMode = WAITCLIMB;
    } else {
      break;
    }

  case WAITCLIMB:
    if (forcecruise) {
      SetCalculated().TurnMode = CRUISE;
      break;
    }
    if ((Rate >= MinTurnRate) || (forcecircling)) {
      if (((Basic().Time - Calculated().TurnStartTime) > CruiseClimbSwitch)
          || forcecircling) {
        // yes, we are certain now that we are circling
        SetCalculated().Circling = true;

        // JMW Transition to climb
        SetCalculated().TurnMode = CLIMB;

        // Remember the start values of the climbing period
        SetCalculated().ClimbStartLocation = Calculated().TurnStartLocation;
        SetCalculated().ClimbStartAlt = Calculated().TurnStartAltitude
            + Calculated().TurnStartEnergyHeight;
        SetCalculated().ClimbStartTime = Calculated().TurnStartTime;

        // QUESTION TB: what is this?
        OnClimbBase(Calculated().TurnStartAltitude);

        // consider code: InputEvents GCE - Move this to InputEvents
        // Consider a way to take the CircleZoom and other logic
        // into InputEvents instead?
        // JMW: NO.  Core functionality must be built into the
        // main program, unable to be overridden.
        OnSwitchClimbMode(true, LEFT);
      }
    } else {
      // nope, not turning, so go back to cruise
      SetCalculated().TurnMode = CRUISE;
    }
    break;

  case CLIMB:
    if ((Rate < MinTurnRate) || (forcecruise)) {
      // Remember the end values of the turn
      SetCalculated().TurnStartTime = Basic().Time;
      SetCalculated().TurnStartLocation = Basic().Location;
      SetCalculated().TurnStartAltitude = Basic().NavAltitude;
      SetCalculated().TurnStartEnergyHeight = Calculated().EnergyHeight;

      // JMW Transition to cruise, due to not properly turning
      SetCalculated().TurnMode = WAITCRUISE;
    }
    if (forcecruise) {
      // QUESTION TB: see above
      SetCalculated().TurnMode = WAITCRUISE;
    } else {
      break;
    }

  case WAITCRUISE:
    if (forcecircling) {
      SetCalculated().TurnMode = CLIMB;
      break;
    }
    if((Rate < MinTurnRate) || forcecruise) {
      if (((Basic().Time - Calculated().TurnStartTime) > ClimbCruiseSwitch)
          || forcecruise) {
        // yes, we are certain now that we are cruising again
        SetCalculated().Circling = false;

        // Transition to cruise
        SetCalculated().TurnMode = CRUISE;
        SetCalculated().CruiseStartLocation = Calculated().TurnStartLocation;
        SetCalculated().CruiseStartAlt = Calculated().TurnStartAltitude;
        SetCalculated().CruiseStartTime = Calculated().TurnStartTime;

        OnClimbCeiling();

        OnSwitchClimbMode(false, LEFT);
      }

      //if ((Basic().Time  - Calculated().TurnStartTime) > ClimbCruiseSwitch/3) {
      // reset thermal locator if changing thermal cores
      // thermallocator.Reset();
      //}
    } else {
      // nope, we are circling again
      // JMW Transition back to climb, because we are turning again
      SetCalculated().TurnMode = CLIMB;
    }
    break;
  default:
    // error, go to cruise
    SetCalculated().TurnMode = CRUISE;
  }
}

void
GlideComputerAirData::ThermalSources()
{
  GEOPOINT ground_location;
  fixed ground_altitude;

  thermallocator.EstimateThermalBase(Calculated().ThermalEstimate_Location,
      Basic().NavAltitude, Calculated().LastThermalAverage,
      Basic().WindSpeed, Basic().WindDirection, &ground_location,
      &ground_altitude);

  if (ground_altitude > 0) {
    double tbest = 0;
    int ibest = 0;

    for (int i = 0; i < MAX_THERMAL_SOURCES; i++) {
      if (Calculated().ThermalSources[i].LiftRate < 0.0) {
        ibest = i;
        break;
      }
      double dt = Basic().Time - Calculated().ThermalSources[i].Time;
      if (dt > tbest) {
        tbest = dt;
        ibest = i;
      }
    }

    SetCalculated().ThermalSources[ibest].LiftRate = Calculated().LastThermalAverage;
    SetCalculated().ThermalSources[ibest].Location = ground_location;
    SetCalculated().ThermalSources[ibest].GroundHeight = ground_altitude;
    SetCalculated().ThermalSources[ibest].Time = Basic().Time;
  }
}

void
GlideComputerAirData::LastThermalStats()
{
  if((Calculated().Circling == false) && (LastCalculated().Circling == true)
     && (Calculated().ClimbStartTime >= 0)) {

    double ThermalTime = Calculated().CruiseStartTime - Calculated().ClimbStartTime;

    if (ThermalTime > 0) {
      double ThermalGain = Calculated().CruiseStartAlt
          + Calculated().EnergyHeight - Calculated().ClimbStartAlt;

      if ((ThermalGain > 0) && (ThermalTime > THERMAL_TIME_MIN)) {

        SetCalculated().LastThermalAverage = ThermalGain / ThermalTime;
        SetCalculated().LastThermalGain = ThermalGain;
        SetCalculated().LastThermalTime = ThermalTime;

        OnDepartedThermal();
      }
    }
  }
}

void
GlideComputerAirData::OnDepartedThermal()
{
  if (SettingsComputer().EnableThermalLocator)
    ThermalSources();
}

void
GlideComputerAirData::ThermalBand()
{
  if (!Basic().Time > LastBasic().Time)
    return;

  // JMW TODO accuracy: Should really work out dt here,
  //           but i'm assuming constant time steps
  double dheight = Basic().NavAltitude
      - SettingsComputer().SafetyAltitudeBreakoff
      - Calculated().TerrainBase; // JMW EXPERIMENTAL

  int index, i, j;

  if (dheight < 0) {
    return; // nothing to do.
  }
  if (Calculated().MaxThermalHeight == 0) {
    SetCalculated().MaxThermalHeight = dheight;
  }

  // only do this if in thermal and have been climbing
  if ((!Calculated().Circling) || (Calculated().Average30s < 0))
    return;

  if (dheight > Calculated().MaxThermalHeight) {
    // moved beyond ceiling, so redistribute buckets
    fixed max_thermal_height_new;
    fixed tmpW[NUMTHERMALBUCKETS];
    int tmpN[NUMTHERMALBUCKETS];
    fixed h;

    // calculate new buckets so glider is below max
    fixed hbuk = Calculated().MaxThermalHeight/NUMTHERMALBUCKETS;

    max_thermal_height_new = max(1.0, Calculated().MaxThermalHeight);
    while (max_thermal_height_new < dheight) {
      max_thermal_height_new += hbuk;
    }

    // reset counters
    for (i = 0; i < NUMTHERMALBUCKETS; i++) {
      tmpW[i]= 0.0;
      tmpN[i]= 0;
    }
    // shift data into new buckets
    for (i = 0; i < NUMTHERMALBUCKETS; i++) {
      h = (i) * (Calculated().MaxThermalHeight) / (NUMTHERMALBUCKETS);
      // height of center of bucket
      j = iround(NUMTHERMALBUCKETS * h / max_thermal_height_new);

      if (j < NUMTHERMALBUCKETS) {
        if (Calculated().ThermalProfileN[i] > 0) {
          tmpW[j] += Calculated().ThermalProfileW[i];
          tmpN[j] += Calculated().ThermalProfileN[i];
        }
      }
    }

    for (i = 0; i < NUMTHERMALBUCKETS; i++) {
      SetCalculated().ThermalProfileW[i] = tmpW[i];
      SetCalculated().ThermalProfileN[i] = tmpN[i];
    }
    SetCalculated().MaxThermalHeight = max_thermal_height_new;
  }

  index = min(NUMTHERMALBUCKETS - 1,
              iround(NUMTHERMALBUCKETS
                     * (dheight / max(1.0, Calculated().MaxThermalHeight))));

  SetCalculated().ThermalProfileW[index] += Calculated().Vario;
  SetCalculated().ThermalProfileN[index]++;
}

void
DoAutoQNH(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated, double takeoffspeedthreshold)
{
  static int done_autoqnh = 0;

  // Reject if already done
  if (done_autoqnh == 10)
    return;

  // Reject if in IGC logger mode
  if (Basic->Replay)
    return;

  // Reject if no valid GPS fix
  if (Basic->NAVWarning)
    return;

  // Reject if no baro altitude
  if (!Basic->BaroAltitudeAvailable)
    return;

  // Reject if terrain height is invalid
  if (!Calculated->TerrainValid)
    return;

  if (Basic->Speed < takeoffspeedthreshold) {
    done_autoqnh++;
  } else {
    done_autoqnh= 0; // restart...
  }

  if (done_autoqnh == 10) {
    double fixaltitude = Calculated->TerrainAlt;
#ifdef OLD_TASK
    Basic->pressure.FindQNH(Basic->BaroAltitude, fixaltitude);
    AllDevicesPutQNH(Basic->pressure);
    airspace_database.set_flight_levels(Basic->pressure);
#endif
  }
}

void
GlideComputerAirData::ProcessSun()
{
  sun.CalcSunTimes(Basic().Location, Basic(), Calculated(), GetUTCOffset() / 3600);
  SetCalculated().TimeSunset = sun.TimeOfSunSet;
}
