/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "InfoBoxes/Content/Factory.hpp"

#include "InfoBoxes/Content/Base.hpp"
#include "InfoBoxes/Content/Alternate.hpp"
#include "InfoBoxes/Content/Altitude.hpp"
#include "InfoBoxes/Content/Direction.hpp"
#include "InfoBoxes/Content/Glide.hpp"
#include "InfoBoxes/Content/MacCready.hpp"
#include "InfoBoxes/Content/Other.hpp"
#include "InfoBoxes/Content/Speed.hpp"
#include "InfoBoxes/Content/Task.hpp"
#include "InfoBoxes/Content/Places.hpp"
#include "InfoBoxes/Content/Contest.hpp"
#include "InfoBoxes/Content/Team.hpp"
#include "InfoBoxes/Content/Terrain.hpp"
#include "InfoBoxes/Content/Thermal.hpp"
#include "InfoBoxes/Content/Time.hpp"
#include "InfoBoxes/Content/Trace.hpp"
#include "InfoBoxes/Content/Weather.hpp"
#include "InfoBoxes/Content/Airspace.hpp"

#include "Util/Macros.hpp"
#include "Language/Language.hpp"

#include <stddef.h>
#include <assert.h>

/**
 * An #InfoBoxContent implementation that invokes a callback.  This is
 * used for those contents that would implement only the Update()
 * method and need no context.
 */
class InfoBoxContentCallback : public InfoBoxContent {
  void (*update)(InfoBoxData &data);
  const InfoBoxPanel *panels;

public:
  InfoBoxContentCallback(void (*_update)(InfoBoxData &data),
                         const InfoBoxPanel *_panels)
    :update(_update), panels(_panels) {}

  virtual void Update(InfoBoxData &data) override {
    update(data);
  }

  virtual const InfoBoxPanel *GetDialogContent() override {
    return panels;
  }
};

template<class T>
struct IBFHelper {
  static InfoBoxContent *Create() {
    return new T();
  }
};

template<class T, int param>
struct IBFHelperInt {
  static InfoBoxContent *Create() {
    return new T(param);
  }
};

using namespace InfoBoxFactory;

struct MetaData {
  const TCHAR *name;
  const TCHAR *caption;
  const TCHAR *description;
  InfoBoxContent *(*create)();
  void (*update)(InfoBoxData &data);
  const InfoBoxPanel *panels;
  Type next, previous;

  /**
   * Implicit instances shall not exist.  This declaration ensures at
   * compile time that the meta_data array is not larger than the
   * number of explicitly initialised elements.
   */
  MetaData() = delete;

  constexpr MetaData(const TCHAR *_name,
                     const TCHAR *_caption,
                     const TCHAR *_description,
                     InfoBoxContent *(*_create)(),
                     Type _next, Type _previous)
    :name(_name), caption(_caption), description(_description),
     create(_create), update(nullptr), panels(nullptr),
     next(_next), previous(_previous) {}

  constexpr MetaData(const TCHAR *_name,
                     const TCHAR *_caption,
                     const TCHAR *_description,
                     void (*_update)(InfoBoxData &data),
                     Type _next, Type _previous)
    :name(_name), caption(_caption), description(_description),
     create(nullptr), update(_update), panels(nullptr),
     next(_next), previous(_previous) {}

  constexpr MetaData(const TCHAR *_name,
                     const TCHAR *_caption,
                     const TCHAR *_description,
                     void (*_update)(InfoBoxData &data),
                     const InfoBoxPanel _panels[],
                     Type _next, Type _previous)
    :name(_name), caption(_caption), description(_description),
     create(nullptr), update(_update), panels(_panels),
     next(_next), previous(_previous) {}
};

// Groups:
//   Altitude: e_HeightGPS,e_HeightAGL,e_H_Terrain,e_H_Baro,e_H_QFE,e_FlightLevel,e_Barogram
//   Aircraft status: e_Speed_GPS,e_Track_GPS,e_AirSpeed_Ext,e_Load_G,e_WP_BearingDiff,e_Speed,e_Horizon
//   GR: e_GR_Instantaneous,e_GR_Cruise,e_Fin_GR_TE,e_Fin_GR,e_WP_GR,e_LD,e_GR_Avg
//   Vario: e_Thermal_30s,e_TL_Avg,e_TL_Gain,e_TL_Time,e_Thermal_Avg,e_Thermal_Gain,e_Climb_Avg,e_VerticalSpeed_GPS,
//          e_VerticalSpeed_Netto,e_Vario_spark,e_NettoVario_spark,e_CirclingAverage_spark,e_ThermalBand,THERMAL_ASSISTANT
//   Wind: e_WindSpeed_Est,e_WindBearing_Est,e_HeadWind,e_Temperature,e_HumidityRel,e_Home_Temperature,WIND_ARROW,HeadWindSimplified
//   MacCready: e_MacCready,e_WP_Speed_MC,e_Climb_Perc,e_Act_Speed,NextLegEqThermal,CruiseEfficiency
//   Nav: e_Bearing,NEXT_RADIAL,e_WP_Distance,e_WP_AltDiff,e_WP_MC0AltDiff,e_WP_H,e_WP_AltReq,e_Fin_AltDiff,e_Fin_AltReq,
//        e_Fin_Distance,e_Home_Distance,
//   Comp&Task: e_SpeedTaskAvg,e_CC_SpeedInst,e_CC_Speed,e_AA_Time,e_AA_TimeDiff,e_AA_DistanceMax,e_AA_DistanceMin,e_AA_SpeedMax,
//              e_AA_SpeedMin,e_Fin_AA_Distance,e_AA_SpeedAvg,e_OC_Distance,e_TaskProgress,
//              START_OPEN_TIME,START_OPEN_ARRIVAL_TIME
//   Waypoint: e_WP_Name,e_TimeSinceTakeoff,e_TimeLocal,e_TimeUTC,e_Fin_Time,e_Fin_ETE_VMG,e_WP_Time,e_WP_ETE_VMG,e_Fin_TimeLocal,
//             e_WP_TimeLocal,e_RH_Trend,e_TaskMaxHeightTime
//   Team: e_Team_Code,e_Team_Bearing,e_Team_BearingDiff,e_Team_Range
//   Gadget: e_Battery,e_CPU_Load
//   Alternates: e_Alternate_1_Name,e_Alternate_2_Name,e_Alternate_1_GR
//   Experimental: e_Experimental1,e_Experimental2
//   Obstacles: e_NearestAirspaceHorizontal,e_NearestAirspaceVertical,TerrainCollision


/* WARNING: Never insert or delete items or rearrange the order of the items
 * in this array. This will break existing infobox configurations of all users!
 */

static constexpr MetaData meta_data[] = {
  // e_HeightGPS
  {
    N_("Altitude GPS"),
    N_("Alt GPS"),
    N_("This is the altitude above mean sea level reported by the GPS. Touch-screen/PC only: In simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn."),
    IBFHelper<InfoBoxContentAltitudeGPS>::Create,
    e_HeightAGL, // H AGL
    e_FlightLevel, // Flight Level
  },

  // e_HeightAGL
  {
    N_("Height AGL"),
    N_("H AGL"),
    N_("This is the navigation altitude minus the terrain elevation obtained from the terrain file. The value is coloured red when the glider is below the terrain safety clearance height."),
    UpdateInfoBoxAltitudeAGL,
    altitude_infobox_panels,
    e_H_Terrain, // Terr Elev
    e_HeightGPS, // Alt GPS
  },

  // e_Thermal_30s
  {
    N_("Thermal climb, last 30 s"),
    N_("TC 30s"),
    N_("A 30 second rolling average climb rate based of the reported GPS altitude, or vario if available."),
    UpdateInfoBoxThermal30s,
    e_TL_Avg, // TL Avg
    e_VerticalSpeed_Netto, // Netto
  },

  // e_Bearing
  {
    N_("Next Bearing"),
    N_("Bearing"),
    N_("True bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector."),
    UpdateInfoBoxBearing,
    next_waypoint_infobox_panels,
    e_Speed_GPS, // V GND
    e_Horizon,
  },

  // e_GR_Instantaneous
  {
    N_("GR instantaneous"),
    N_("GR Inst"),
    N_("Instantaneous glide ratio over ground, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds. Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---'."),
    UpdateInfoBoxGRInstant,
    e_GR_Cruise, // GR Cruise
    e_GR_Avg, // GR Avg
  },

  // e_GR_Cruise
  {
    N_("GR cruise"),
    N_("GR Cruise"),
    N_("The distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal. Negative values indicate climbing cruise (height gain since leaving the last thermal). If the vertical speed is close to zero, the displayed value is '---'."),
    UpdateInfoBoxGRCruise,
    e_Fin_GR_TE, // Final GR
    e_GR_Instantaneous, // GR Inst
  },

  // e_Speed_GPS
  {
    N_("Speed ground"),
    N_("V GND"),
    N_("Ground speed measured by the GPS. If this InfoBox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider."),
    IBFHelper<InfoBoxContentSpeedGround>::Create,
    e_Track_GPS, // Track
    e_Bearing, // Bearing
  },

  // e_TL_Avg
  {
    N_("Last thermal average"),
    N_("TL Avg"),
    N_("Total altitude gain/loss in the last thermal divided by the time spent circling."),
    UpdateInfoBoxThermalLastAvg,
    e_TL_Gain, // TL Gain
    e_Thermal_30s, // TC 30s
  },

  // e_TL_Gain
  {
    N_("Last thermal gain"),
    N_("TL Gain"),
    N_("Total altitude gain/loss in the last thermal."),
    UpdateInfoBoxThermalLastGain,
    e_TL_Time, // TL Time
    e_TL_Avg, // TL Avg
  },

  // e_TL_Time
  {
    N_("Last thermal time"),
    N_("TL Time"),
    N_("Time spent circling in the last thermal."),
    UpdateInfoBoxThermalLastTime,
    e_Thermal_Avg, // TC Avg
    e_TL_Gain, // TL Gain
  },

  // e_MacCready
  {
    N_("MacCready setting"),
    N_("MC"),
    N_("The current MacCready setting and the current MacCready mode (manual or auto). (Touch-screen/PC only) Also used to adjust the MacCready setting if the InfoBox is active, by using the up/down cursor keys."),
    IBFHelper<InfoBoxContentMacCready>::Create,
    e_WP_Speed_MC, // V MC
    NextLegEqThermal,
  },

  // e_WP_Distance
  {
    N_("Next distance"),
    N_("WP Dist"),
    N_("The distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector."),
    UpdateInfoBoxNextDistance,
    next_waypoint_infobox_panels,
    WP_NOMINAL_DIST, // WP Dist-N
    e_TaskProgress, // Progress
  },

  // e_WP_AltDiff
  {
    N_("Next altitude difference"),
    N_("WP AltD"),
    N_("Arrival altitude at the next waypoint relative to the safety arrival height. For AAT tasks, the target within the AAT sector is used."),
    UpdateInfoBoxNextAltitudeDiff,
    next_waypoint_infobox_panels,
    e_WP_MC0AltDiff, // WP MC0 AltD
    WP_NOMINAL_DIST, // WP Dist-N
  },

  // e_WP_AltReq
  {
    N_("Next altitude required"),
    N_("WP AltR"),
    N_("Additional altitude required to reach the next turn point. For AAT tasks, the target within the AAT sector is used."),
    UpdateInfoBoxNextAltitudeRequire,
    next_waypoint_infobox_panels,
    e_Fin_AltDiff, // Fin AltD
    e_WP_AltDiff, // WP AltD
  },

  // e_WP_Name
  {
    N_("Next waypoint"),
    N_("Next WP"),
    N_("The name of the currently selected turn point. When this InfoBox is active, using the up/down cursor keys selects the next/previous waypoint in the task. (Touch-screen/PC only) Pressing the enter cursor key brings up the waypoint details."),
    IBFHelper<InfoBoxContentNextWaypoint>::Create,
    e_TimeSinceTakeoff, // Time flt
    e_TaskMaxHeightTime, // Start height
  },

  // e_Fin_AltDiff
  {
    N_("Final altitude difference"),
    N_("Fin AltD"),
    N_("Arrival altitude at the final task turn point relative to the safety arrival height."),
    UpdateInfoBoxFinalAltitudeDiff,
    e_Fin_AltReq, // Fin AltR
    e_WP_AltReq, // WP AltR
  },

  // e_Fin_AltReq
  {
    N_("Final altitude required"),
    N_("Fin AltR"),
    N_("Additional altitude required to finish the task."),
    UpdateInfoBoxFinalAltitudeRequire,
    e_SpeedTaskAvg, // V Task Av
    e_Fin_AltDiff, // Fin AltD
  },

  // e_SpeedTaskAvg
  {
    N_("Speed task average"),
    N_("V Task Avg"),
    N_("Average cross country speed while on current task, not compensated for altitude."),
    UpdateInfoBoxTaskSpeed,
    e_CC_SpeedInst, // V Task Inst
    e_Fin_AltReq, // Fin AltR
  },

  // e_Fin_Distance
  {
    N_("Final distance"),
    N_("Fin Dist"),
    N_("Distance to finish around remaining turn points."),
    UpdateInfoBoxFinalDistance,
    e_AA_Time, // AA Time
    TASK_SPEED_HOUR,
  },

  // e_Fin_GR_TE
  {
    _T("Final GR (TE) deprecated"),
    _T("---"),
    _T("Deprecated, there is no TE compensation on GR, you should switch to the \"Final GR\" info box."),
    UpdateInfoBoxFinalGR,
    e_Fin_GR, // Final GR
    e_GR_Cruise, // GR Cruise
  },

  // e_H_Terrain
  {
    N_("Terrain elevation"),
    N_("Terr Elev"),
    N_("This is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location."),
    UpdateInfoBoxTerrainHeight,
    e_H_Baro, // H Baro
    e_HeightAGL, // H AGL
  },

  // e_Thermal_Avg
  {
    N_("Thermal average"),
    N_("TC Avg"),
    N_("Altitude gained/lost in the current thermal, divided by time spent thermalling."),
    UpdateInfoBoxThermalAvg,
    e_Thermal_Gain, // TC Gain
    e_TL_Time, // TL Time
  },

  // e_Thermal_Gain
  {
    N_("Thermal gain"),
    N_("TC Gain"),
    N_("The altitude gained/lost in the current thermal."),
    UpdateInfoBoxThermalGain,
    e_Climb_Avg, // TC All
    e_Thermal_Avg, // TC Avg
  },

  // e_Track_GPS
  {
    N_("Track"),
    N_("Track"),
    N_("Magnetic track reported by the GPS. (Touch-screen/PC only) If this InfoBox is active in simulation mode, pressing the up and down  arrows adjusts the track."),
    IBFHelper<InfoBoxContentTrack>::Create,
    e_AirSpeed_Ext, // V IAS
    e_Speed_GPS, // V GND
  },

  // e_VerticalSpeed_GPS
  {
    N_("Vario"),
    N_("Vario"),
    N_("Instantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    UpdateInfoBoxVario,
    e_VerticalSpeed_Netto, // Netto
    e_Climb_Avg, // TC All
  },

  // e_WindSpeed_Est
  {
    N_("Wind speed"),
    N_("Wind V"),
    N_("Wind speed estimated by XCSoar. Manual adjustment is possible with the connected InfoBox dialogue. Pressing the up/down cursor keys to cycle through settings, adjust the values with left/right cursor keys."),
    UpdateInfoBoxWindSpeed,
    wind_infobox_panels,
    e_WindBearing_Est, // Wind B
    e_Home_Temperature, // Max Temp
  },

  // e_WindBearing_Est
  {
    N_("Wind bearing"),
    N_("Wind Brng"),
    N_("Wind bearing estimated by XCSoar. Manual adjustment is possible with the connected InfoBox dialogue. Pressing the up/down cursor keys to cycle through settings, adjust the values with left/right cursor keys."),
    UpdateInfoBoxWindBearing,
    wind_infobox_panels,
    HeadWindSimplified,
    e_WindSpeed_Est, // Wind V
  },

  // e_AA_Time
  {
    N_("AAT time"),
    N_("AAT Time"),
    N_("Assigned Area Task time remaining. Goes red when time remaining has expired."),
    UpdateInfoBoxTaskAATime,
    e_AA_TimeDiff, // AA dTime
    e_Fin_Distance, // Fin Dis
  },

  // e_AA_DistanceMax
  {
    N_("AAT max. distance "),
    N_("AAT Dmax"),
    N_("Assigned Area Task maximum distance possible for remainder of task."),
    UpdateInfoBoxTaskAADistanceMax,
    e_AA_DistanceMin, // AA Dmin
    e_AA_TimeDiff, // AA dTime
  },

  // e_AA_DistanceMin
  {
    N_("AAT min. distance"),
    N_("AAT Dmin"),
    N_("Assigned Area Task minimum distance possible for remainder of task."),
    UpdateInfoBoxTaskAADistanceMin,
    e_AA_SpeedMax, // AA Vmax
    e_AA_DistanceMax, // AA Dmax
  },

  // e_AA_SpeedMax
  {
    N_("AAT speed max. distance"),
    N_("AAT Vmax"),
    N_("Assigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time."),
    UpdateInfoBoxTaskAASpeedMax,
    e_AA_SpeedMin, // AA Vmin
    e_AA_DistanceMin, // AA Dmin
  },

  // e_AA_SpeedMin
  {
    N_("AAT speed min. distance"),
    N_("AAT Vmin"),
    N_("Assigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time."),
    UpdateInfoBoxTaskAASpeedMin,
    e_Fin_AA_Distance, // AA Dtgt
    e_AA_SpeedMax, // AA Vmax
  },

  // e_AirSpeed_Ext
  {
    N_("Airspeed IAS"),
    N_("V IAS"),
    N_("Indicated Airspeed reported by a supported external intelligent vario."),
    UpdateInfoBoxSpeedIndicated,
    e_Load_G, // G load
    e_Track_GPS, // Track
  },

  // e_H_Baro
  {
    N_("Barometric altitude"),
    N_("Alt Baro"),
    N_("This is the barometric altitude obtained from a device equipped with a pressure sensor."),
    UpdateInfoBoxAltitudeBaro,
    altitude_infobox_panels,
    e_H_QFE, // H T/O
    e_H_Terrain, // Terr Elev
  },

  // e_WP_Speed_MC
  {
    N_("Speed MacCready"),
    N_("V MC"),
    N_("The MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent."),
    UpdateInfoBoxSpeedMacCready,
    e_Climb_Perc, // % Climb
    e_MacCready, // MC
  },

  // e_Climb_Perc
  {
    N_("Percentage climb"),
    N_("% Climb"),
    N_("Percentage of time spent in climb mode. These statistics are reset upon starting the task."),
    UpdateInfoBoxThermalRatio,
    e_Act_Speed, // V Opt
    e_WP_Speed_MC, // V MC
  },

  // e_TimeSinceTakeoff
  {
    N_("Flight Duration"),
    N_("Flt Duration"),
    N_("Time elapsed since takeoff was detected."),
    UpdateInfoBoxTimeFlight,
    e_TimeLocal, // Time local
    e_WP_Name, // Next
  },

  // e_Load_G
  {
    N_("G load"),
    N_("G"),
    N_("Magnitude of G loading reported by a supported external intelligent vario. This value is negative for pitch-down manoeuvres."),
    UpdateInfoBoxGLoad,
    e_WP_BearingDiff, // Bearing D
    e_AirSpeed_Ext, // Track
  },

  // e_WP_GR
  {
    N_("Next GR"),
    N_("WP GR"),
    N_("The required glide ratio over ground to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival height."),
    UpdateInfoBoxNextGR,
    next_waypoint_infobox_panels,
    e_LD, // LD Vario
    e_Fin_GR, // Final GR
  },

  // e_TimeLocal
  {
    N_("Time local"),
    N_("Time loc"),
    N_("GPS time expressed in local time zone."),
    UpdateInfoBoxTimeLocal,
    e_TimeUTC, // Time UTC
    e_TimeSinceTakeoff, // Time flt
  },

  // e_TimeUTC
  {
    N_("Time UTC"),
    N_("Time UTC"),
    N_("GPS time expressed in UTC."),
    UpdateInfoBoxTimeUTC,
    e_Fin_Time, // Fin ETE
    e_TimeLocal, // Time local
  },

  // e_Fin_Time
  {
    N_("Task time to go"),
    N_("Fin ETE"),
    N_("Estimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle."),
    UpdateInfoBoxFinalETE,
    e_Fin_ETE_VMG,
    e_TimeUTC, // Time UTC
  },

  // e_WP_Time
  {
    N_("Next time to go"),
    N_("WP ETE"),
    N_("Estimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    UpdateInfoBoxNextETE,
    next_waypoint_infobox_panels,
    e_WP_ETE_VMG,
    e_Fin_ETE_VMG
  },

  // e_Act_Speed
  {
    N_("Speed dolphin"),
    N_("Vopt"),
    N_("The instantaneous MacCready speed-to-fly, making use of netto vario calculations to determine dolphin cruise speed in the glider's current bearing. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent. In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected). When Block mode speed to fly is selected, this InfoBox displays the MacCready speed."),
    UpdateInfoBoxSpeedDolphin,
    NextLegEqThermal,
    e_Climb_Perc // % Climb
  },

  // e_VerticalSpeed_Netto
  {
    N_("Netto vario"),
    N_("Netto"),
    N_("Instantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate. Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates."),
    UpdateInfoBoxVarioNetto,
    e_Vario_spark, // Vario trace
    e_VerticalSpeed_GPS, // Vario
  },

  // e_Fin_TimeLocal
  {
    N_("Task arrival time"),
    N_("Fin ETA"),
    N_("Estimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle."),
    UpdateInfoBoxFinalETA,
    e_WP_TimeLocal, // WP ETA
    e_WP_ETE_VMG,
  },

  // e_WP_TimeLocal
  {
    N_("Next arrival time"),
    N_("WP ETA"),
    N_("Estimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    UpdateInfoBoxNextETA,
    next_waypoint_infobox_panels,
    e_RH_Trend, // RH Trend
    e_Fin_TimeLocal, // Fin ETA
  },

  // e_WP_BearingDiff
  {
    N_("Bearing difference"),
    N_("Brng D"),
    N_("The difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector. GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present. Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint. This bearing takes into account the curvature of the Earth."),
    UpdateInfoBoxBearingDiff,
    e_Speed, // V TAS
    e_Load_G, // G load
  },

  // e_Temperature
  {
    N_("Outside air temperature"),
    N_("OAT"),
    N_("Outside air temperature measured by a probe if supported by a connected intelligent variometer."),
    UpdateInfoBoxTemperature,
    e_HumidityRel, // RelHum
    e_HeadWind,
  },

  // e_HumidityRel
  {
    N_("Relative humidity"),
    N_("Rel Hum"),
    N_("Relative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer."),
    UpdateInfoBoxHumidity,
    e_Home_Temperature, // MaxTemp
    e_Temperature, // OAT
  },

  // e_Home_Temperature
  {
    N_("Forecast temperature"),
    N_("Max Temp"),
    N_("Forecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe. (Touch-screen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature."),
    IBFHelper<InfoBoxContentTemperatureForecast>::Create,
    e_WindSpeed_Est, // Wind V
    e_HumidityRel, // RelHum
  },

  // e_Fin_AA_Distance
  {
    N_("AAT distance around target"),
    N_("AAT Dtgt"),
    N_("Assigned Area Task distance around target points for remainder of task."),
    UpdateInfoBoxTaskAADistance,
    e_AA_SpeedAvg, // AA Vtgt
    e_AA_SpeedMin, // AA Vmin
  },

  // e_AA_SpeedAvg
  {
    N_("AAT speed around target"),
    N_("AAT Vtgt"),
    N_("Assigned Area Task average speed achievable around target points remaining in minimum AAT time."),
    UpdateInfoBoxTaskAASpeed,
    e_Home_Distance, // Home Dis
    e_Fin_AA_Distance, // AA Dtgt
  },

  // e_LD
  {
    N_("L/D vario"),
    N_("L/D Vario"),
    N_("Instantaneous lift/drag ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer. Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---'."),
    UpdateInfoBoxLDVario,
    e_GR_Avg, // GR Avg
    e_WP_GR, // Next GR
  },

  // e_Speed
  {
    N_("Airspeed TAS"),
    N_("V TAS"),
    N_("True Airspeed reported by a supported external intelligent vario."),
    UpdateInfoBoxSpeed,
    e_Horizon,
    e_WP_BearingDiff, // Bearing Diff
  },

  // e_Team_Code
  {
    N_("Team code"),
    N_("Team Code"),
    N_("The current Team code for this aircraft. Use this to report to other team members. The last team aircraft code entered is displayed underneath."),
    IBFHelper<InfoBoxContentTeamCode>::Create,
    e_Team_Bearing, // Team Bearing
    e_Team_Range, // Team Range
  },

  // e_Team_Bearing
  {
    N_("Team bearing"),
    N_("Team Brng"),
    N_("The bearing to the team aircraft location at the last team code report."),
    UpdateInfoBoxTeamBearing,
    e_Team_BearingDiff, // Team Bearing Diff
    e_Team_Code, // Team Code
  },

  // e_Team_BearingDiff
  {
    N_("Team bearing difference"),
    N_("Team BrngD"),
    N_("The relative bearing to the team aircraft location at the last reported team code."),
    UpdateInfoBoxTeamBearingDiff,
    e_Team_Range, // Team Range
    e_Team_Bearing, // Team Bearing
  },

  // e_Team_Range
  {
    N_("Team range"),
    N_("Team Dist"),
    N_("The range to the team aircraft location at the last reported team code."),
    UpdateInfoBoxTeamDistance,
    e_Team_Code, // Team Code
    e_Team_BearingDiff, // Team Bearing Diff
  },

  // e_CC_SpeedInst
  {
    N_("Speed task instantaneous"),
    N_("V Task Inst"),
    N_("Instantaneous cross country speed while on current task, compensated for altitude. Equivalent to instantaneous Pirker cross-country speed."),
    UpdateInfoBoxTaskSpeedInstant,
    e_CC_Speed, // V Task Ach
    e_SpeedTaskAvg, // V Task Av
  },

  // e_Home_Distance
  {
    N_("Distance home"),
    N_("Home Dist"),
    N_("Distance to home waypoint (if defined)."),
    UpdateInfoBoxHomeDistance,
    e_OC_Distance, // OLC
    e_AA_SpeedAvg, // AA Vtgt
  },

  // e_CC_Speed
  {
    N_("Speed task achieved"),
    N_("V Task Ach"),
    N_("Achieved cross country speed while on current task, compensated for altitude.  Equivalent to Pirker cross-country speed remaining."),
    UpdateInfoBoxTaskSpeedAchieved,
    TASK_SPEED_HOUR,
    e_CC_SpeedInst, // V Task Inst
  },

  // e_AA_TimeDiff
  {
    N_("AAT delta time"),
    N_("AAT dT"),
    N_("Difference between estimated task time and AAT minimum time. Coloured red if negative (expected arrival too early), or blue if in sector and can turn now with estimated arrival time greater than AAT time plus 5 minutes."),
    UpdateInfoBoxTaskAATimeDelta,
    e_AA_DistanceMax, // AA Dmax
    e_AA_Time, // AA Time
  },

  // e_Climb_Avg
  {
    N_("Thermal average over all"),
    N_("T Avg"),
    N_("Time-average climb rate in all thermals."),
    UpdateInfoBoxThermalAllAvg,
    e_VerticalSpeed_GPS, // Vario
    e_Thermal_Gain, // TC Gain
  },

  // e_RH_Trend
  {
    N_("Task req. total height trend"),
    N_("RH Trend"),
    N_("Trend (or neg. of the variation) of the total required height to complete the task."),
    UpdateInfoBoxVarioDistance,
    e_TaskMaxHeightTime, // Start height
    e_WP_TimeLocal, // WP ETA
  },

  // e_Battery
  {
#ifndef GNAV
    N_("Battery percent"),
#else
    N_("Battery voltage"),
#endif
    N_("Battery"),
    N_("Displays percentage of device battery remaining (where applicable) and status/voltage of external power supply."),
    UpdateInfoBoxBattery,
    e_CPU_Load, // CPU
    e_CPU_Load, // CPU
  },

  // e_Fin_GR
  {
    N_("Final GR"),
    N_("Fin GR"),
    N_("The required glide ratio over ground to finish the task, given by the distance to go divided by the height required to arrive at the safety arrival height."),
    UpdateInfoBoxFinalGR,
    e_WP_GR, // Next GR
    e_Fin_GR_TE, // Fin GR
  },

  // e_Alternate_1_Name
  {
    N_("Alternate 1"),
    N_("Altn 1"),
    N_("Displays name and bearing to the best alternate landing location."),
    IBFHelperInt<InfoBoxContentAlternateName, 0>::Create,
    e_Alternate_2_Name, // Altern2 name
    e_Alternate_1_GR, // Altern1 GR
  },

  // e_Alternate_2_Name
  {
    N_("Alternate 2"),
    N_("Altn 2"),
    N_("Displays name and bearing to the second alternate landing location."),
    IBFHelperInt<InfoBoxContentAlternateName, 1>::Create,
    e_Alternate_1_GR, // Altern1 GR
    e_Alternate_1_Name, // Altern1 name
  },

  // e_Alternate_1_GR
  {
    N_("Alternate 1 GR"),
    N_("Altn1 GR"),
    N_("Geometric gradient to the arrival height above the best alternate. This is not adjusted for total energy."),
    IBFHelperInt<InfoBoxContentAlternateGR, 0>::Create,
    e_Alternate_1_Name, // Altern1 name
    e_Alternate_2_Name, // Altern2 name
  },

  // e_H_QFE
  {
    N_("Height above take-off"),
    N_("H T/O"),
    N_("Height based on an automatic take-off reference elevation (like a QFE reference)."),
    UpdateInfoBoxAltitudeQFE,
    altitude_infobox_panels,
    e_FlightLevel, // Flight Level
    e_H_Baro, // Alt Baro
  },

  // e_GR_Avg
  {
    N_("GR average"),
    N_("GR Avg"),
    N_("The distance made in the configured period of time , divided by the altitude lost since then. Negative values are shown as ^^^ and indicate climbing cruise (height gain). Over 200 of GR the value is shown as +++ . You can configure the period of averaging in the system setup. Suggested values are 60, 90 or 120. Lower values will be closed to GR Inst, and higher values will be closed to GR Cruise. Notice that the distance is NOT the straight line between your old and current position, it's exactly the distance you have made even in a zigzag glide. This value is not calculated while circling."),
    UpdateInfoBoxGRAvg,
    e_GR_Instantaneous, // GR Inst
    e_LD, // LD Vario
  },

  // e_Experimental
  {
    N_("Experimental 1"),
    N_("Exp1"),
    NULL,
    UpdateInfoBoxExperimental1,
    e_Experimental2, // Exp2
    e_Experimental2, // Exp2
  },

  // e_OC_Distance
  {
    N_("On-Line Contest distance"),
    N_("OLC"),
    N_("Instantaneous evaluation of the flown distance according to the configured On-Line Contest rule set."),
    IBFHelper<InfoBoxContentOLC>::Create,
    e_TaskProgress, // Progress
    e_Home_Distance, // Home Dis
  },

  // e_Experimental2
  {
    N_("Experimental 2"),
    N_("Exp2"),
    NULL,
    UpdateInfoBoxExperimental2,
    e_Experimental1, // Exp1
    e_Experimental1, // Exp1
  },

  // e_CPU_Load
  {
    N_("CPU load"),
    N_("CPU"),
    N_("CPU load consumed by XCSoar averaged over 5 seconds."),
    UpdateInfoBoxCPULoad,
    e_Battery, // Battery
    e_Battery, // Battery
  },

  // e_WP_H
  {
    N_("Next altitude arrival"),
    N_("WP AltA"),
    N_("Absolute arrival altitude at the next waypoint in final glide.  For AAT tasks, the target within the AAT sector is used."),
    UpdateInfoBoxNextAltitudeArrival,
    next_waypoint_infobox_panels,
    e_WP_AltReq, // WP AltR
    e_WP_AltDiff, // WP AltD
  },

  // e_Free_RAM
  {
    N_("Free RAM"),
    N_("Free RAM"),
    N_("Free RAM as reported by OS."),
    UpdateInfoBoxFreeRAM,
    e_CPU_Load, // CPU Load
    e_CPU_Load, // CPU Load
  },

  // e_FlightLevel
  {
    N_("Flight level"),
    N_("FL"),
    N_("Pressure Altitude given as Flight Level. Only available if barometric altitude available and correct QNH set."),
    UpdateInfoBoxAltitudeFlightLevel,
    altitude_infobox_panels,
    e_Barogram, // Barogram
    e_H_QFE, // H T/O
  },

  // e_Barogram
  {
    N_("Barogram"),
    N_("Barogram"),
    N_("Trace of altitude during flight"),
    IBFHelper<InfoBoxContentBarogram>::Create,
    e_HeightGPS, // H GPS
    e_FlightLevel, // Flight level
  },

  // e_Vario_spark
  {
    N_("Vario trace"),
    N_("Vario Trace"),
    N_("Trace of vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    IBFHelper<InfoBoxContentVarioSpark>::Create,
    e_NettoVario_spark, // Netto trace
    e_VerticalSpeed_Netto, // Netto
  },

  // e_NettoVario_spark
  {
    N_("Netto vario trace"),
    N_("Netto Trace"),
    N_("Trace of vertical speed of air-mass, equal to vario value less the glider's estimated sink rate."),
    IBFHelper<InfoBoxContentNettoVarioSpark>::Create,
    e_CirclingAverage_spark, // TC trace
    e_Vario_spark, // Vario trace
  },
  
  // e_CirclingAverage_spark
  {
    N_("Thermal climb trace"),
    N_("TC Trace"),
    N_("Trace of average climb rate each turn in circling, based of the reported GPS altitude, or vario if available."),
    IBFHelper<InfoBoxContentCirclingAverageSpark>::Create,
    e_ThermalBand, // Climb band
    e_NettoVario_spark, // Netto trace
  },

  // e_ThermalBand
  {
    N_("Climb band"),
    N_("Climb Band"),
    N_("Graph of average circling climb rate (horizontal axis) as a function of altitude (vertical axis)."),
    IBFHelper<InfoBoxContentThermalBand>::Create,
    e_Thermal_30s, // TC 30s
    e_CirclingAverage_spark, // TC trace
  },

  // e_TaskProgress
  {
    N_("Task progress"),
    N_("Progress"),
    N_("Clock-like display of distance remaining along task, showing achieved task points."),
    IBFHelper<InfoBoxContentTaskProgress>::Create,
    e_WP_Distance, // WP Dist
    e_OC_Distance, // OLC
  },

  // e_TaskMaxHeightTime
  {
    N_("Time under max. start height"),
    N_("Start Height"),
    N_("The contiguous period the ship has been below the task start max. height."),
    UpdateInfoBoxTaskTimeUnderMaxHeight,
    e_WP_Name, // Next WP
    e_RH_Trend, // RH Trend
  },

  // e_Fin_ETE_VMG
  {
    N_("Task time to go (ground speed)"),
    N_("Fin ETE VMG"),
    N_("Estimated time required to complete task, assuming current ground speed is maintained."),
    UpdateInfoBoxFinalETEVMG,
    e_WP_Time, // WP ETE
    e_Fin_Time,
  },

  // e_WP_ETE_VMG
  {
    N_("Next time to go (ground speed)"),
    N_("WP ETE VMG"),
    N_("Estimated time required to reach next waypoint, assuming current ground speed is maintained."),
    UpdateInfoBoxNextETEVMG,
    next_waypoint_infobox_panels,
    e_Fin_TimeLocal, // Fin ETA
    e_WP_Time,
  },

  // e_Horizon
  {
    N_("Attitude indicator"),
    N_("Horizon"),
    N_("Attitude indicator (artificial horizon) display calculated from flight path, supplemented with acceleration and variometer data if available."),
    IBFHelper<InfoBoxContentHorizon>::Create,
    e_Bearing,
    e_Speed,
  },

  // e_NearestAirspaceHorizontal
  {
    N_("Nearest airspace horizontal"),
    N_("Near AS H"),
    N_("The horizontal distance to the nearest airspace."),
    UpdateInfoBoxNearestAirspaceHorizontal,
    e_NearestAirspaceVertical,
    TerrainCollision,
  },

  // e_NearestAirspaceVertical
  {
    N_("Nearest airspace vertical"),
    N_("Near AS V"),
    N_("The vertical distance to the nearest airspace.  A positive value means the airspace is above you, and negative means the airspace is below you."),
    UpdateInfoBoxNearestAirspaceVertical,
    TerrainCollision,
    e_NearestAirspaceHorizontal,
  },

  // e_WP_MC0AltDiff
  {
    N_("Next MC0 altitude difference"),
    N_("WP MC0 AltD"),
    N_("Arrival altitude at the next waypoint with MC 0 setting relative to the safety arrival height.  For AAT tasks, the target within the AAT sector is used."),
    UpdateInfoBoxNextMC0AltitudeDiff,
    next_waypoint_infobox_panels,
    e_WP_H, // WP AltA
    e_WP_AltDiff, // WP AltD
  },

  // e_HeadWind
  {
    N_("Head wind component"),
    N_("Head Wind"),
    N_("The current head wind component. Head wind is calculated from TAS and GPS ground speed if airspeed is available from external device. Otherwise the estimated wind is used for the calculation."),
    UpdateInfoBoxHeadWind,
    wind_infobox_panels,
    e_Temperature, // OAT
    HeadWindSimplified,
  },

  // TerrainCollision
  {
    N_("Terrain collision"),
    N_("Terr Coll"),
    N_("The distance to the next terrain collision along the current task leg. At this location, the altitude will be below the configured terrain clearance altitude."),
    UpdateInfoBoxTerrainCollision,
    e_NearestAirspaceHorizontal,
    e_NearestAirspaceVertical,
  },

  {
    N_("Altitude (Auto)"),
    N_("Alt Auto"),
    N_("This is the barometric altitude obtained from a device equipped with a pressure sensor or the GPS altitude if the barometric altitude is not available."),
    UpdateInfoBoxAltitudeNav,
    altitude_infobox_panels,
    NavAltitude,
    NavAltitude,
  },

  // NextLegEqThermal
  {
    N_("Thermal next leg equivalent"),
    N_("T Next Leg"),
    N_("The thermal rate of climb on next leg which is equivalent to a thermal equal to the MacCready setting on current leg."),
    UpdateInfoBoxNextLegEqThermal,
    e_MacCready, // MC
    e_Act_Speed, // V Opt
  },

  // HeadWindSimplified
  {
    N_("Head wind component (simplified)"),
    N_("Head Wind *"),
    N_("The current head wind component. The simplified head wind is calculated by subtracting GPS ground speed from the TAS if airspeed is available from external device."),
    UpdateInfoBoxHeadWindSimplified,
    wind_infobox_panels,
    e_HeadWind, // OAT
    e_WindBearing_Est,
  },

  {
    N_("Task cruise efficiency"),
    N_("Cruise Eff"),
    N_("Efficiency of cruise.  100 indicates perfect MacCready performance. "
       "This value estimates your cruise efficiency according to the current "
       "flight history with the set MC value.  Calculation begins after task is started."),
    UpdateInfoBoxCruiseEfficiency,
    CruiseEfficiency,
    CruiseEfficiency,
  },

  {
    N_("Wind arrow"),
    N_("Wind"),
    N_("Wind speed estimated by XCSoar. Manual adjustment is possible with the connected InfoBox dialogue. Pressing the up/down cursor keys to cycle through settings, adjust the values with left/right cursor keys."),
    IBFHelper<InfoBoxContentWindArrow>::Create,
    WIND_ARROW,
    WIND_ARROW,
  },

  {
    N_("Thermal assistant"),
    N_("Thermal"),
    N_("A circular thermal assistant that shows the lift distribution over each part of the circle."),
    IBFHelper<InfoBoxContentThermalAssistant>::Create,
    THERMAL_ASSISTANT,
    THERMAL_ASSISTANT,
  },


  {
    N_("Start open/close countdown"),
    N_("Start open"),
    N_("Shows the time left until the start point opens or closes."),
    UpdateInfoBoxStartOpen,
    START_OPEN_ARRIVAL_TIME,
    START_OPEN_ARRIVAL_TIME,
  },

  {
    N_("Start open/close countdown at reaching"),
    N_("Start reach"),
    N_("Shows the time left until the start point opens or closes, compared to the calculated time to reach it."),
    UpdateInfoBoxStartOpenArrival,
    START_OPEN_TIME,
    START_OPEN_TIME,
  },

  {
    N_("Next radial"),
    N_("Radial"),
    N_("True bearing from the next waypoint to your position."),
    UpdateInfoBoxRadial,
    next_waypoint_infobox_panels,
    ATC_RADIAL,
    ATC_RADIAL,
  },

  {
    N_("ATC radial"),
    N_("ATC radial"),
    N_("True bearing from the selected reference location to your position.  The distance is displayed in nautical miles for communication with ATC."),
    UpdateInfoBoxATCRadial,
    atc_infobox_panels,
    NEXT_RADIAL,
    NEXT_RADIAL,
  },


  {
    N_("Speed task last hour"),
    N_("V Task H"),
    N_("Average cross country speed while on current task over the last hour, not compensated for altitude."),
    UpdateInfoBoxTaskSpeedHour,
    e_Fin_Distance,
    e_CC_Speed,
  },

  // WP_NOMINAL_DIST
  {
    N_("Next distance (nominal)"),
    N_("WP Dist-N"),
    N_("The distance to the currently selected waypoint. For AAT tasks, this is the distance to the origin of the AAT sector."),
    UpdateInfoBoxNextDistanceNominal,
    next_waypoint_infobox_panels,
    e_WP_AltDiff, // WP AltD
    e_WP_Distance, // WP Dist
  },

  {
    N_("Circle Diameter"),
    N_("Circle D"),
    N_("Cirle diameter. Displays estimated circle diameter and full circle flight time. Useful for evaluating best thermalling mode with a glider at different wing loading."),
    UpdateInfoBoxCircleDiameter,
    CIRCLE_DIAMETER,
    CIRCLE_DIAMETER,
  },

};

static_assert(ARRAY_SIZE(meta_data) == NUM_TYPES,
              "Wrong InfoBox factory size");

const TCHAR *
InfoBoxFactory::GetName(Type type)
{
  assert(type < NUM_TYPES);

  return meta_data[type].name;
}

const TCHAR *
InfoBoxFactory::GetCaption(Type type)
{
  assert(type < NUM_TYPES);

  return meta_data[type].caption;
}

/**
 * Returns the long description (help text) of the info box type.
 */
const TCHAR *
InfoBoxFactory::GetDescription(Type type)
{
  assert(type < NUM_TYPES);

  return meta_data[type].description;
}

InfoBoxFactory::Type
InfoBoxFactory::GetNext(Type type)
{
  assert(type < NUM_TYPES);

  return meta_data[type].next;
}

InfoBoxFactory::Type
InfoBoxFactory::GetPrevious(Type type)
{
  assert(type < NUM_TYPES);

  return meta_data[type].previous;
}

InfoBoxContent*
InfoBoxFactory::Create(Type type)
{
  assert(type < NUM_TYPES);
  const auto &m = meta_data[type];

  assert(m.create != nullptr ||
         m.update != nullptr);

  if (m.create != nullptr)
    return m.create();
  else
    return new InfoBoxContentCallback(m.update, m.panels);
}
