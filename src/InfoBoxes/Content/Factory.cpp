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
#include "InfoBoxes/Content/Team.hpp"
#include "InfoBoxes/Content/Thermal.hpp"
#include "InfoBoxes/Content/Time.hpp"
#include "InfoBoxes/Content/Trace.hpp"
#include "InfoBoxes/Content/Weather.hpp"
#include "InfoBoxes/Content/Airspace.hpp"

#include "Language/Language.hpp"

#include "Profile/ProfileMap.hpp"

#include <stddef.h>

// Groups:
//   Altitude: e_HeightGPS,e_HeightAGL,e_H_Terrain,e_H_Baro,e_H_QFE,e_FlightLevel
//   Aircraft info: e_Bearing,e_Speed_GPS,e_Track_GPS,e_AirSpeed_Ext,e_Load_G,e_WP_BearingDiff,e_Speed
//   LD: e_LD_Instantaneous,e_LD_Cruise,e_Fin_LD,e_Fin_GR,e_WP_LD,e_LD,e_LD_Avg
//   Vario: e_Thermal_30s,e_TL_Avg,e_TL_Gain,e_TL_Time,e_Thermal_Avg,e_Thermal_Gain,e_Climb_Avg,e_VerticalSpeed_GPS,e_VerticalSpeed_Netto
//   Wind: e_WindSpeed_Est,e_WindBearing_Est,e_Temperature,e_HumidityRel,e_Home_Temperature
//   MacCready: e_MacCready,e_WP_Speed_MC,e_Climb_Perc,e_Act_Speed
//   Nav: e_WP_Distance,e_WP_AltDiff,e_WP_H,e_WP_AltReq,e_Fin_AltDiff,e_Fin_AltReq,e_SpeedTaskAvg,59,61,e_Fin_Distance,e_AA_Time,e_AA_DistanceMax,e_AA_DistanceMin,e_AA_SpeedMax,e_AA_SpeedMin,e_Fin_AA_Distance,e_AA_SpeedAvg,60,73
//   Waypoint: e_WP_Name,e_TimeSinceTakeoff,e_TimeLocal,e_TimeUTC,e_Fin_Time,e_WP_Time,e_Fin_TimeLocal,e_WP_TimeLocal,e_RH_Trend
//   Team: e_Team_Code,e_Team_Bearing,e_Team_BearingDiff,e_Team_Range
//   Gadget: e_Battery,e_CPU_Load
//   Alternates: e_Alternate_1_Name,e_Alternate_2_Name,e_Alternate_1_GR
//   Experimental: e_Experimental1,e_Experimental2
const InfoBoxFactory::InfoBoxMetaData InfoBoxFactory::MetaData[NUM_TYPES] = {
  // e_HeightGPS
  {
    N_("Height GPS"),
    N_("H GPS"),
    N_("This is the height above mean sea level reported by the GPS. Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn."),
    e_HeightAGL, // H AGL
    e_FlightLevel, // Flight Level
  },

  // e_HeightAGL
  {
    N_("Height AGL"),
    N_("H AGL"),
    N_("This is the navigation altitude minus the terrain height obtained from the terrain file. The value is coloured red when the glider is below the terrain safety clearance height."),
    e_H_Terrain, // H Gnd
    e_HeightGPS, // H GPS
  },

  // e_Thermal_30s
  {
    N_("Thermal last 30 s"),
    N_("TC 30s"),
    N_("A 30 second rolling average climb rate based of the reported GPS altitude, or vario if available."),
    e_TL_Avg, // TL Avg
    e_VerticalSpeed_Netto, // Netto
  },

  // e_Bearing
  {
    N_("Bearing"),
    N_("Bearing"),
    N_("True bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector."),
    e_Speed_GPS, // V Gnd
    e_Speed, // V TAS
  },

  // e_LD_Instantaneous
  {
    N_("L/D instantaneous"),
    N_("L/D Inst"),
    N_("Instantaneous glide ratio, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds. Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---'."),
    e_LD_Cruise, // LD Cruise
    e_LD_Avg, // LD Avg
  },

  // e_LD_Cruise
  {
    N_("L/D cruise"),
    N_("L/D Cru"),
    N_("The distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal. Negative values indicate climbing cruise (height gain since leaving the last thermal). If the vertical speed is close to zero, the displayed value is '---'."),
    e_Fin_LD, // Final LD
    e_LD_Instantaneous, // LD Inst
  },

  // e_Speed_GPS
  {
    N_("Speed ground"),
    N_("V Gnd"),
    N_("Ground speed measured by the GPS. If this InfoBox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider."),
    e_Track_GPS, // Track
    e_Bearing, // Bearing
  },

  // e_TL_Avg
  {
    N_("Last thermal average"),
    N_("TL Avg"),
    N_("Total altitude gain/loss in the last thermal divided by the time spent circling."),
    e_TL_Gain, // TL Gain
    e_Thermal_30s, // TC 30s
  },

  // e_TL_Gain
  {
    N_("Last thermal gain"),
    N_("TL Gain"),
    N_("Total altitude gain/loss in the last thermal."),
    e_TL_Time, // TL Time
    e_TL_Avg, // TL Avg
  },

  // e_TL_Time
  {
    N_("Last thermal time"),
    N_("TL Time"),
    N_("Time spent circling in the last thermal."),
    e_Thermal_Avg, // TC Avg
    e_TL_Gain, // TL Gain
  },

  // e_MacCready
  {
    N_("MacCready setting"),
    N_("MC"),
    N_("The current MacCready setting. This InfoBox also shows whether MacCready is manual or auto. (Touchscreen/PC only) Also used to adjust the MacCready Setting if the InfoBox is active, by using the up/down cursor keys."),
    e_WP_Speed_MC, // V MC
    e_Act_Speed, // V Opt
  },

  // e_WP_Distance
  {
    N_("Next distance"),
    N_("WP Dist"),
    N_("The distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector."),
    e_WP_AltDiff, // WP AltD
    e_OC_Distance, // OLC
  },

  // e_WP_AltDiff
  {
    N_("Next altitude difference"),
    N_("WP AltD"),
    N_("Arrival altitude at the next waypoint relative to the safety arrival height."),
    e_WP_H, // WP AltA
    e_WP_Distance, // WP Dist
  },

  // e_WP_AltReq
  {
    N_("Next altitude required"),
    N_("WP AltR"),
    N_("Additional altitude required to reach the next turn point."),
    e_Fin_AltDiff, // Fin AltD
    e_WP_AltDiff, // WP AltD
  },

  // e_WP_Name
  {
    N_("Next waypoint"),
    N_("Next"),
    N_("The name of the currently selected turn point. When this InfoBox is active, using the up/down cursor keys selects the next/previous waypoint in the task. (Touchscreen/PC only) Pressing the enter cursor key brings up the waypoint details."),
    e_TimeSinceTakeoff, // Time flt
    e_RH_Trend, // RH Trend
  },

  // e_Fin_AltDiff
  {
    N_("Final altitude difference"),
    N_("Fin AltD"),
    N_("Arrival altitude at the final task turn point relative to the safety arrival height."),
    e_Fin_AltReq, // Fin AltR
    e_WP_AltReq, // WP AltR
  },

  // e_Fin_AltReq
  {
    N_("Final altitude required"),
    N_("Fin AltR"),
    N_("Additional altitude required to finish the task."),
    e_SpeedTaskAvg, // V Task Av
    e_Fin_AltDiff, // Fin AltD
  },

  // e_SpeedTaskAvg
  {
    N_("Speed task average"),
    N_("VTask Avg"),
    N_("Average cross country speed while on current task, not compensated for altitude."),
    e_CC_SpeedInst, // V Task Inst
    e_Fin_AltReq, // Fin AltR
  },

  // e_Fin_Distance
  {
    N_("Final distance"),
    N_("Fin Dist"),
    N_("Distance to finish around remaining turn points."),
    e_AA_Time, // AA Time
    e_CC_Speed, // V Task Ach
  },

  // e_Fin_LD
  {
    N_("Final L/D"),
    N_("Fin LD"),
    N_("The required glide ratio to finish the task, given by the distance to go divided by the height required to arrive at the safety arrival height. Negative values indicate a climb is necessary to finish. If the height required is close to zero, the displayed value is '---'. Note that this calculation may be optimistic because it reduces the height required to finish by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds."),
    e_Fin_GR, // Final GR
    e_LD_Cruise, // LD Cruise
  },

  // e_H_Terrain
  {
    N_("Terrain elevation"),
    N_("H GND"),
    N_("This is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location."),
    e_H_Baro, // H Baro
    e_HeightAGL, // H AGL
  },

  // e_Thermal_Avg
  {
    N_("Thermal average"),
    N_("TC Avg"),
    N_("Altitude gained/lost in the current thermal, divided by time spent thermaling."),
    e_Thermal_Gain, // TC Gain
    e_TL_Time, // TL Time
  },

  // e_Thermal_Gain
  {
    N_("Thermal gain"),
    N_("TC Gain"),
    N_("The altitude gained/lost in the current thermal."),
    e_Climb_Avg, // TC All
    e_Thermal_Avg, // TC Avg
  },

  // e_Track_GPS
  {
    N_("Track"),
    N_("Track"),
    N_("Magnetic track reported by the GPS. (Touchscreen/PC only) If this InfoBox is active in simulation mode, pressing the up and down  arrows adjusts the track."),
    e_AirSpeed_Ext, // V IAS
    e_Speed_GPS, // V Gnd
  },

  // e_VerticalSpeed_GPS
  {
    N_("Vario"),
    N_("Vario"),
    N_("Instantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    e_VerticalSpeed_Netto, // Netto
    e_Climb_Avg, // TC All
  },

  // e_WindSpeed_Est
  {
    N_("Wind speed"),
    N_("Wind V"),
    N_("Wind speed estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust magnitude and left/right cursor keys to adjust bearing when the InfoBox is active. Pressing the enter cursor key saves the wind value as the initial value when XCSoar next starts."),
    e_WindBearing_Est, // Wind B
    e_Home_Temperature, // Max Temp
  },

  // e_WindBearing_Est
  {
    N_("Wind bearing"),
    N_("Wind B"),
    N_("Wind bearing estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust bearing when the InfoBox is active."),
    e_Temperature, // OAT
    e_WindSpeed_Est, // Wind V
  },

  // e_AA_Time
  {
    N_("AA time"),
    N_("AA Time"),
    N_("Assigned Area Task time remaining. Goes red when time remaining has expired."),
    e_AA_TimeDiff, // AA dTime
    e_Fin_Distance, // Fin Dis
  },

  // e_AA_DistanceMax
  {
    N_("AA max. distance "),
    N_("AA Dmax"),
    N_("Assigned Area Task maximum distance possible for remainder of task."),
    e_AA_DistanceMin, // AA Dmin
    e_AA_TimeDiff, // AA dTime
  },

  // e_AA_DistanceMin
  {
    N_("AA min. distance"),
    N_("AA Dmin"),
    N_("Assigned Area Task minimum distance possible for remainder of task."),
    e_AA_SpeedMax, // AA Vmax
    e_AA_DistanceMax, // AA Dmax
  },

  // e_AA_SpeedMax
  {
    N_("AA speed max. distance"),
    N_("AA Vmax"),
    N_("Assigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time."),
    e_AA_SpeedMin, // AA Vmin
    e_AA_DistanceMin, // AA Dmin
  },

  // e_AA_SpeedMin
  {
    N_("AA speed min. distance"),
    N_("AA Vmin"),
    N_("Assigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time."),
    e_Fin_AA_Distance, // AA Dtgt
    e_AA_SpeedMax, // AA Vmax
  },

  // e_AirSpeed_Ext
  {
    N_("Airspeed IAS"),
    N_("V IAS"),
    N_("Indicated Airspeed reported by a supported external intelligent vario."),
    e_Load_G, // G load
    e_Track_GPS, // Track
  },

  // e_H_Baro
  {
    N_("Barometric altitude"),
    N_("Alt. Baro."),
    N_("This is the barometric altitude obtained from a device equipped with a pressure sensor."),
    e_H_QFE, // QFE GPS
    e_H_Terrain, // H Gnd
  },

  // e_WP_Speed_MC
  {
    N_("Speed MacCready"),
    N_("V MC"),
    N_("The MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent."),
    e_Climb_Perc, // % Climb
    e_MacCready, // MC
  },

  // e_Climb_Perc
  {
    N_("Percentage climb"),
    N_("% Climb"),
    N_("Percentage of time spent in climb mode. These statistics are reset upon starting the task."),
    e_Act_Speed, // V Opt
    e_WP_Speed_MC, // V MC
  },

  // e_TimeSinceTakeoff
  {
    N_("Time of flight"),
    N_("Time Flt"),
    N_("Time elapsed since takeoff was detected."),
    e_TimeLocal, // Time local
    e_WP_Name, // Next
  },

  // e_Load_G
  {
    N_("G load"),
    N_("G"),
    N_("Magnitude of G loading reported by a supported external intelligent vario. This value is negative for pitch-down manoeuvres."),
    e_WP_BearingDiff, // Bearing D
    e_AirSpeed_Ext, // Track
  },

  // e_WP_LD
  {
    N_("Next L/D"),
    N_("WP LD"),
    N_("The required glide ratio to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival height. Negative values indicate a climb is necessary to reach the waypoint. If the height required is close to zero, the displayed value is '---'.   Note that this calculation may be optimistic because it reduces the height required to reach the waypoint by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds."),
    e_LD, // LD Vario
    e_Fin_GR, // Final GR
  },

  // e_TimeLocal
  {
    N_("Time local"),
    N_("Time loc"),
    N_("GPS time expressed in local time zone."),
    e_TimeUTC, // Time UTC
    e_TimeSinceTakeoff, // Time flt
  },

  // e_TimeUTC
  {
    N_("Time UTC"),
    N_("Time UTC"),
    N_("GPS time expressed in UTC."),
    e_Fin_Time, // Fin ETE
    e_TimeLocal, // Time local
  },

  // e_Fin_Time
  {
    N_("Task time to go"),
    N_("Fin ETE"),
    N_("Estimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle."),
    e_WP_Time, // WP ETE
    e_TimeUTC, // Time UTC
  },

  // e_WP_Time
  {
    N_("Next time to go"),
    N_("WP ETE"),
    N_("Estimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    e_Fin_TimeLocal, // Fin ETA
    e_Fin_Time, // Fin ETE
  },

  // e_Act_Speed
  {
    N_("Speed dolphin"),
    N_("V Opt"),
    N_("The instantaneous MacCready speed-to-fly, making use of Netto vario calculations to determine dolphin cruise speed in the glider's current bearing. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent. In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected). When Block mode speed to fly is selected, this InfoBox displays the MacCready speed."),
    e_MacCready, // MC
    e_Climb_Perc, // % Climb
  },

  // e_VerticalSpeed_Netto
  {
    N_("Netto vario"),
    N_("Netto"),
    N_("Instantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate. Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates."),
    e_Thermal_30s, // TC 30s
    e_VerticalSpeed_GPS, // Vario
  },

  // e_Fin_TimeLocal
  {
    N_("Task arrival time"),
    N_("Fin ETA"),
    N_("Estimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle."),
    e_WP_TimeLocal, // WP ETA
    e_WP_Time, // WP ETE
  },

  // e_WP_TimeLocal
  {
    N_("Next arrival time"),
    N_("WP ETA"),
    N_("Estimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    e_RH_Trend, // RH Trend
    e_Fin_TimeLocal, // Fin ETA
  },

  // e_WP_BearingDiff
  {
    N_("Bearing difference"),
    N_("Brng D"),
    N_("The difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector. GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present. Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint. This bearing takes into account the curvature of the Earth."),
    e_Speed, // V TAS
    e_Load_G, // G load
  },

  // e_Temperature
  {
    N_("Outside air temperature"),
    N_("OAT"),
    N_("Outside air temperature measured by a probe if supported by a connected intelligent variometer."),
    e_HumidityRel, // RelHum
    e_WindBearing_Est, // Wind B
  },

  // e_HumidityRel
  {
    N_("Relative humidity"),
    N_("Rel Hum"),
    N_("Relative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer."),
    e_Home_Temperature, // MaxTemp
    e_Temperature, // OAT
  },

  // e_Home_Temperature
  {
    N_("Forecast temperature"),
    N_("Max Temp"),
    N_("Forecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe. (Touchscreen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature."),
    e_WindSpeed_Est, // Wind V
    e_HumidityRel, // RelHum
  },

  // e_Fin_AA_Distance
  {
    N_("AA distance around target"),
    N_("AA Dtgt"),
    N_("Assigned Area Task distance around target points for remainder of task."),
    e_AA_SpeedAvg, // AA Vtgt
    e_AA_SpeedMin, // AA Vmin
  },

  // e_AA_SpeedAvg
  {
    N_("AA speed around target"),
    N_("AA Vtgt"),
    N_("Assigned Area Task average speed achievable around target points remaining in minimum AAT time."),
    e_Home_Distance, // Home Dis
    e_Fin_AA_Distance, // AA Dtgt
  },

  // e_LD
  {
    N_("L/D vario"),
    N_("LD Vario"),
    N_("Instantaneous glide ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer. Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---'."),
    e_LD_Avg, // LD Avg
    e_WP_LD, // Next LD
  },

  // e_Speed
  {
    N_("Airspeed TAS"),
    N_("V TAS"),
    N_("True Airspeed reported by a supported external intelligent vario."),
    e_Bearing, // Bearing
    e_WP_BearingDiff, // Bearing Diff
  },

  // e_Team_Code
  {
    N_("Team code"),
    N_("TeamCode"),
    N_("The current Team code for this aircraft. Use this to report to other team members. The last team aircraft code entered is displayed underneath."),
    e_Team_Bearing, // Team Bearing
    e_Team_Range, // Team Range
  },

  // e_Team_Bearing
  {
    N_("Team bearing"),
    N_("Tm Brng"),
    N_("The bearing to the team aircraft location at the last team code report."),
    e_Team_BearingDiff, // Team Bearing Diff
    e_Team_Code, // Team Code
  },

  // e_Team_BearingDiff
  {
    N_("Team bearing difference"),
    N_("Team Bd"),
    N_("The relative bearing to the team aircraft location at the last reported team code."),
    e_Team_Range, // Team Range
    e_Team_Bearing, // Team Bearing
  },

  // e_Team_Range
  {
    N_("Team range"),
    N_("Team Dis"),
    N_("The range to the team aircraft location at the last reported team code."),
    e_Team_Code, // Team Code
    e_Team_BearingDiff, // Team Bearing Diff
  },

  // e_CC_SpeedInst
  {
    N_("Speed task instantaneous"),
    N_("V Tsk Ins"),
    N_("Instantaneous cross country speed while on current task, compensated for altitude. Equivalent to instantaneous Pirker cross-country speed."),
    e_CC_Speed, // V Task Ach
    e_SpeedTaskAvg, // V Task Av
  },

  // e_Home_Distance
  {
    N_("Distance home"),
    N_("Home Dis"),
    N_("Distance to home waypoint (if defined)."),
    e_OC_Distance, // OLC
    e_AA_SpeedAvg, // AA Vtgt
  },

  // e_CC_Speed
  {
    N_("Speed task achieved"),
    N_("V Tsk Ach"),
    N_("Achieved cross country speed while on current task, compensated for altitude.  Equivalent to Pirker cross-country speed remaining."),
    e_Fin_Distance, // Fin Dis
    e_CC_SpeedInst, // V Task Inst
  },

  // e_AA_TimeDiff
  {
    N_("AA delta time"),
    N_("AA dT"),
    N_("Difference between estimated task time and AAT minimum time. Colored red if negative (expected arrival too early), or blue if in sector and can turn now with estimated arrival time greater than AAT time plus 5 minutes."),
    e_AA_DistanceMax, // AA Dmax
    e_AA_Time, // AA Time
  },

  // e_Climb_Avg
  {
    N_("Thermal average over all"),
    N_("TC All"),
    N_("Time-average climb rate in all thermals."),
    e_VerticalSpeed_GPS, // Vario
    e_Thermal_Gain, // TC Gain
  },

  // e_RH_Trend
  {
    N_("Task req. total height trend"),
    N_("RH Trend"),
    N_("Trend (or neg. of the variation) of the total required height to complete the task."),
    e_WP_Name, // Next
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
    e_CPU_Load, // CPU
    e_CPU_Load, // CPU
  },

  // e_Fin_GR
  {
    N_("Final GR"),
    N_("Fin GR"),
    N_("Geometric gradient to the arrival height above the final waypoint. This is not adjusted for total energy."),
    e_WP_LD, // Next LD
    e_Fin_LD, // Fin LD
  },

  // e_Alternate_1_Name
  {
    N_("Alternate 1 name"),
    N_("Altrn 1"),
    N_("Displays name and bearing to the best alternate landing location."),
    e_Alternate_2_Name, // Altern2 name
    e_Alternate_1_GR, // Altern1 GR
  },

  // e_Alternate_2_Name
  {
    N_("Alternate 2 name"),
    N_("Altrn 2"),
    N_("Displays name and bearing to the second alternate landing location."),
    e_Alternate_1_GR, // Altern1 GR
    e_Alternate_1_Name, // Altern1 name
  },

  // e_Alternate_1_GR
  {
    N_("Alternate 1 GR"),
    N_("Altrn1 GR"),
    N_("Geometric gradient to the arrival height above the best alternate. This is not adjusted for total energy."),
    e_Alternate_1_Name, // Altern1 name
    e_Alternate_2_Name, // Altern2 name
  },

  // e_H_QFE
  {
    N_("QFE GPS"),
    N_("QFE GPS"),
    N_("Automatic QFE. This altitude value is constantly reset to 0 on ground BEFORE taking off. After takeoff, it is no more reset automatically even if on ground. During flight you can change QFE with up and down keys. Bottom line shows QNH altitude. Changing QFE does not affect QNH altitude."),
    e_FlightLevel, // Flight Level
    e_H_Baro, // H Baro
  },

  // e_LD_Avg
  {
    N_("L/D average"),
    N_("L/D Avg"),
    N_("The distance made in the configured period of time , divided by the altitude lost since then. Negative values are shown as ^^^ and indicate climbing cruise (height gain). Over 200 of LD the value is shown as +++ . You can configure the period of averaging in the Special config menu. Suggested values for this configuration are 60, 90 or 120: lower values will be closed to LD INST, and higher values will be closed to LD Cruise. Notice that the distance is NOT the straight line between your old and current position: it's exactly the distance you have made even in a zigzag glide. This value is not calculated while circling. "),
    e_LD_Instantaneous, // LD Inst
    e_LD, // LD Vario
  },

  // e_Experimental
  {
    N_("Experimental 1"),
    N_("Exp1"),
    NULL,
    e_Experimental2, // Exp2
    e_Experimental2, // Exp2
  },

  // e_OC_Distance
  {
    N_("On-Line Contest distance"),
    N_("OLC"),
    N_("Instantaneous evaluation of the flown distance according to the configured On-Line Contest rule set."),
    e_WP_Distance, // WP Dist
    e_Home_Distance, // Home Dis
  },

  // e_Experimental2
  {
    N_("Experimental 2"),
    N_("Exp2"),
    NULL,
    e_Experimental1, // Exp1
    e_Experimental1, // Exp1
  },

  // e_CPU_Load
  {
    N_("CPU load"),
    N_("CPU"),
    N_("CPU load consumed by XCSoar averaged over 5 seconds."),
    e_Battery, // Battery
    e_Battery, // Battery
  },

  // e_WP_H
  {
    N_("Next altitude arrival"),
    N_("WP AltA"),
    N_("Absolute arrival altitude at the next waypoint in final glide."),
    e_WP_AltReq, // WP AltR
    e_WP_AltDiff, // WP AltD
  },

  // e_Free_RAM
  {
    N_("Free RAM"),
    N_("Free RAM"),
    N_("Free RAM as reported by OS."),
    e_CPU_Load, // CPU Load
    e_CPU_Load, // CPU Load
  },

  // e_FlightLevel
  {
    N_("Flight level"),
    N_("Flight Level"),
    N_("Pressure Altitude given as Flight Level. Only available if barometric altitude available and correct QNH set."),
    e_HeightGPS, // H GPS
    e_H_QFE, // QFE GPS
  },

  // e_Barogram
  {
    N_("Barogram"),
    N_("Barogram"),
    N_("Trace of altitude during flight"),
    e_Barogram, // H GPS
    e_Barogram, // QFE GPS
  },

  // e_Vario_spark
  {
    N_("Vario trace"),
    N_("Vario"),
    N_("Trace of vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    e_Barogram, // H GPS
    e_Barogram, // QFE GPS
  },

  // e_NettoVario_spark
  {
    N_("Netto vario trace"),
    N_("Netto"),
    N_("Trace of vertical speed of air-mass, equal to vario value less the glider's estimated sink rate."),
    e_Barogram, // H GPS
    e_Barogram, // QFE GPS
  },
  
  // e_CirclingAverage_spark
  {
    N_("Thermal circling trace"),
    N_("TC Circling"),
    N_("Trace of average climb rate each turn in circling, based of the reported GPS altitude, or vario if available."),
    e_Barogram, // H GPS
    e_Barogram, // QFE GPS
  },

  // e_ThermalBand
  {
    N_("Climb band"),
    N_("Climb band"),
    N_("Graph of average circling climb rate (horizontal axis) as a function of height (vertical axis)."),
    e_Barogram, // H GPS
    e_Barogram, // QFE GPS
  },

  // e_TaskProgress
  {
    N_("Task progress"),
    N_("Progress"),
    N_("Clock-like display of distance remaining along task, showing achieved taskpoints."),
    e_Barogram, // H GPS
    e_Barogram, // QFE GPS
  },

  // e_TaskMaxHeightTime
  {
    N_("Time under max. start height"),
    N_("Start Height"),
    N_("The contiguous period the ship has been below the task start max. height."),
    e_TaskMaxHeightTime,
    e_TaskMaxHeightTime,
  },

  // e_Fin_ETE_VMG
  {
    N_("Task time to go (gnd spd)"),
    N_("Fin ETE VMG"),
    N_("Estimated time required to complete task, assuming current ground speed is maintained."),
    e_WP_Time, // WP ETE
    e_TimeUTC, // Time UTC
  },

  // e_WP_ETE_VMG
  {
    N_("Next time to go (gnd spd)"),
    N_("WP ETE VMG"),
    N_("Estimated time required to reach next waypoint, assuming current ground speed is maintained."),
    e_Fin_TimeLocal, // Fin ETA
    e_Fin_Time, // Fin ETE
  },

  {
    N_("Attitude indicator"),
    N_("Horizon"),
    N_("Attitude indicator (artifical horizon) display calculated from flightpath, supplemented with acceleration and variometer data if available."),
    e_Fin_TimeLocal, // Fin ETA
    e_Fin_Time, // Fin ETE
  },

  {
    N_("Nearest airspace horizontal"),
    N_("Near AS H"),
    N_("The horizontal distance to the nearest airspace"),
    e_NearestAirspaceVertical,
    e_NearestAirspaceVertical,
  },

  {
    N_("Nearest airspace vertical"),
    N_("Near AS V"),
    N_("The vertical distance to the nearest airspace.  A positive value means the airspace is above you, and negative means the airspace is below you."),
    e_NearestAirspaceHorizontal,
    e_NearestAirspaceHorizontal,
  },
};

bool
InfoBoxFactory::Get(const TCHAR *key, InfoBoxFactory::t_InfoBox &val)
{
  unsigned _val = val;
  bool ret = ProfileMap::Get(key, _val);
  val = (InfoBoxFactory::t_InfoBox)_val;
  return ret;
}

InfoBoxContent*
InfoBoxFactory::Create(t_InfoBox InfoBoxType)
{
  switch (InfoBoxType) {
  case e_HeightGPS:
    return new InfoBoxContentAltitudeGPS();
  case e_HeightAGL:
    return new InfoBoxContentAltitudeAGL();
  case e_Thermal_30s:
    return new InfoBoxContentThermal30s();
  case e_Bearing:
    return new InfoBoxContentBearing();
  case e_LD_Instantaneous:
    return new InfoBoxContentLDInstant();
  case e_LD_Cruise:
    return new InfoBoxContentLDCruise();
  case e_Speed_GPS:
    return new InfoBoxContentSpeedGround();
  case e_TL_Avg:
    return new InfoBoxContentThermalLastAvg();
  case e_TL_Gain:
    return new InfoBoxContentThermalLastGain();
  case e_TL_Time:
    return new InfoBoxContentThermalLastTime();
  case e_MacCready:
    return new InfoBoxContentMacCready();
  case e_WP_Distance:
    return new InfoBoxContentNextDistance();
  case e_WP_AltDiff:
    return new InfoBoxContentNextAltitudeDiff();
  case e_WP_AltReq:
    return new InfoBoxContentNextAltitudeRequire();
  case e_WP_Name:
    return new InfoBoxContentNextWaypoint();
  case e_Fin_AltDiff:
    return new InfoBoxContentFinalAltitudeDiff();
  case e_Fin_AltReq:
    return new InfoBoxContentFinalAltitudeRequire();
  case e_SpeedTaskAvg:
    return new InfoBoxContentTaskSpeed();
  case e_Fin_Distance:
    return new InfoBoxContentFinalDistance();
  case e_Fin_LD:
    return new InfoBoxContentFinalLD();
  case e_H_Terrain:
    return new InfoBoxContentTerrainHeight();
  case e_Thermal_Avg:
    return new InfoBoxContentThermalAvg();
  case e_Thermal_Gain:
    return new InfoBoxContentThermalGain();
  case e_Track_GPS:
    return new InfoBoxContentTrack();
  case e_VerticalSpeed_GPS:
    return new InfoBoxContentVario();
  case e_WindSpeed_Est:
    return new InfoBoxContentWindSpeed();
  case e_WindBearing_Est:
    return new InfoBoxContentWindBearing();
  case e_AA_Time:
    return new InfoBoxContentTaskAATime();
  case e_AA_DistanceMax:
    return new InfoBoxContentTaskAADistanceMax();
  case e_AA_DistanceMin:
    return new InfoBoxContentTaskAADistanceMin();
  case e_AA_SpeedMax:
    return new InfoBoxContentTaskAASpeedMax();
  case e_AA_SpeedMin:
    return new InfoBoxContentTaskAASpeedMin();
  case e_AirSpeed_Ext:
    return new InfoBoxContentSpeedIndicated();
  case e_H_Baro:
    return new InfoBoxContentAltitudeBaro();
  case e_WP_Speed_MC:
    return new InfoBoxContentSpeedMacCready();
  case e_Climb_Perc:
    return new InfoBoxContentThermalRatio();
  case e_TimeSinceTakeoff:
    return new InfoBoxContentTimeFlight();
  case e_Load_G:
    return new InfoBoxContentGLoad();
  case e_WP_LD:
    return new InfoBoxContentNextLD();
  case e_TimeLocal:
    return new InfoBoxContentTimeLocal();
  case e_TimeUTC:
    return new InfoBoxContentTimeUTC();
  case e_Fin_Time:
    return new InfoBoxContentFinalETE();
  case e_WP_Time:
    return new InfoBoxContentNextETE();
  case e_Act_Speed:
    return new InfoBoxContentSpeedDolphin();
  case e_VerticalSpeed_Netto:
    return new InfoBoxContentVarioNetto();
  case e_Fin_TimeLocal:
    return new InfoBoxContentFinalETA();
  case e_WP_TimeLocal:
    return new InfoBoxContentNextETA();
  case e_WP_BearingDiff:
    return new InfoBoxContentBearingDiff();
  case e_Temperature:
    return new InfoBoxContentTemperature();
  case e_HumidityRel:
    return new InfoBoxContentHumidity();
  case e_Home_Temperature:
    return new InfoBoxContentTemperatureForecast();
  case e_Fin_AA_Distance:
    return new InfoBoxContentTaskAADistance();
  case e_AA_SpeedAvg:
    return new InfoBoxContentTaskAASpeed();
  case e_LD:
    return new InfoBoxContentLDVario();
  case e_Speed:
    return new InfoBoxContentSpeed();
  case e_Team_Code:
    return new InfoBoxContentTeamCode();
  case e_Team_Bearing:
    return new InfoBoxContentTeamBearing();
  case e_Team_BearingDiff:
    return new InfoBoxContentTeamBearingDiff();
  case e_Team_Range:
    return new InfoBoxContentTeamDistance();
  case e_CC_SpeedInst:
    return new InfoBoxContentTaskSpeedInstant();
  case e_Home_Distance:
    return new InfoBoxContentHomeDistance();
  case e_CC_Speed:
    return new InfoBoxContentTaskSpeedAchieved();
  case e_AA_TimeDiff:
    return new InfoBoxContentTaskAATimeDelta();
  case e_Climb_Avg:
    return new InfoBoxContentThermalAllAvg();
  case e_RH_Trend:
    return new InfoBoxContentVarioDistance();
  case e_Battery:
    return new InfoBoxContentBattery();
  case e_Fin_GR:
    return new InfoBoxContentFinalGR();
  case e_Alternate_1_Name:
    return new InfoBoxContentAlternateName(0);
  case e_Alternate_2_Name:
    return new InfoBoxContentAlternateName(1);
  case e_Alternate_1_GR:
    return new InfoBoxContentAlternateGR(0);
  case e_H_QFE:
    return new InfoBoxContentAltitudeQFE();
  case e_LD_Avg:
    return new InfoBoxContentLDAvg();
  case e_Experimental1:
    return new InfoBoxContentExperimental1();
  case e_OC_Distance:
    return new InfoBoxContentOLC();
  case e_Experimental2:
    return new InfoBoxContentExperimental2();
  case e_CPU_Load:
    return new InfoBoxContentCPULoad();
  case e_Barogram:
    return new InfoBoxContentBarogram();
  case e_WP_H:
    return new InfoBoxContentNextAltitudeArrival();
  case e_Free_RAM:
    return new InfoBoxContentFreeRAM();
  case e_FlightLevel:
    return new InfoBoxContentFlightLevel();
  case e_Vario_spark:
    return new InfoBoxContentVarioSpark();
  case e_NettoVario_spark:
    return new InfoBoxContentNettoVarioSpark();
  case e_CirclingAverage_spark:
    return new InfoBoxContentCirclingAverageSpark();
  case e_ThermalBand:
    return new InfoBoxContentThermalBand();
  case e_TaskProgress:
    return new InfoBoxContentTaskProgress();
  case e_TaskMaxHeightTime:
    return new InfoBoxContentTaskTimeUnderMaxHeight();
  case e_Fin_ETE_VMG:
    return new InfoBoxContentFinalETEVMG();
  case e_WP_ETE_VMG:
    return new InfoBoxContentNextETEVMG();
  case e_Horizon:
    return new InfoBoxContentHorizon();

  case e_NearestAirspaceHorizontal:
    return new InfoBoxContentNearestAirspaceHorizontal();

  case e_NearestAirspaceVertical:
    return new InfoBoxContentNearestAirspaceVertical();

  default:
    return NULL;
  }
}
