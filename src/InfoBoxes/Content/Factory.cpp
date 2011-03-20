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

#include "Language.hpp"

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
  // 0
  {
    N_("Height GPS"),
    N_("H GPS"),
    N_("This is the height above mean sea level reported by the GPS. Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn."),
    e_HeightAGL, // H AGL
    e_FlightLevel, // Flight Level
  },

  // 1
  {
    N_("Height AGL"),
    N_("H AGL"),
    N_("This is the navigation altitude minus the terrain height obtained from the terrain file. The value is coloured red when the glider is below the terrain safety clearance height."),
    e_H_Terrain, // H Gnd
    e_HeightGPS, // H GPS
  },

  // 2
  {
    N_("Thermal last 30 sec"),
    N_("TC 30s"),
    N_("A 30 second rolling average climb rate based of the reported GPS altitude, or vario if available."),
    e_TL_Avg, // TL Avg
    e_VerticalSpeed_Netto, // Netto
  },

  // 3
  {
    N_("Bearing"),
    N_("Bearing"),
    N_("True bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector."),
    e_Speed_GPS, // V Gnd
    e_Speed, // V TAS
  },

  // 4
  {
    N_("L/D instantaneous"),
    N_("L/D Inst"),
    N_("Instantaneous glide ratio, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds. Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---'."),
    e_LD_Cruise, // LD Cruise
    71, // LD Avg
  },

  // 5
  {
    N_("L/D cruise"),
    N_("L/D Cru"),
    N_("The distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal. Negative values indicate climbing cruise (height gain since leaving the last thermal). If the vertical speed is close to zero, the displayed value is '---'."),
    e_Fin_LD, // Final LD
    e_LD_Instantaneous, // LD Inst
  },

  // 6
  {
    N_("Speed ground"),
    N_("V Gnd"),
    N_("Ground speed measured by the GPS. If this infobox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider."),
    e_Track_GPS, // Track
    e_Bearing, // Bearing
  },

  // 7
  {
    N_("Last Thermal Average"),
    N_("TL Avg"),
    N_("Total altitude gain/loss in the last thermal divided by the time spent circling."),
    e_TL_Gain, // TL Gain
    e_Thermal_30s, // TC 30s
  },

  // 8
  {
    N_("Last Thermal Gain"),
    N_("TL Gain"),
    N_("Total altitude gain/loss in the last thermal."),
    e_TL_Time, // TL Time
    e_TL_Avg, // TL Avg
  },

  // 9
  {
    N_("Last Thermal Time"),
    N_("TL Time"),
    N_("Time spent circling in the last thermal."),
    e_Thermal_Avg, // TC Avg
    e_TL_Gain, // TL Gain
  },

  // 10
  {
    N_("MacCready Setting"),
    N_("MacCready"),
    N_("The current MacCready setting. This infobox also shows whether MacCready is manual or auto. (Touchscreen/PC only) Also used to adjust the MacCready Setting if the infobox is active, by using the up/down cursor keys."),
    e_WP_Speed_MC, // V MC
    e_Act_Speed, // V Opt
  },

  // 11
  {
    N_("Next Distance"),
    N_("WP Dist"),
    N_("The distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector."),
    e_WP_AltDiff, // WP AltD
    73, // OLC
  },

  // 12
  {
    N_("Next Altitude Difference"),
    N_("WP AltD"),
    N_("Arrival altitude at the next waypoint relative to the safety arrival altitude."),
    76, // WP AltA
    e_WP_Distance, // WP Dist
  },

  // 13
  {
    N_("Next Altitude Required"),
    N_("WP AltR"),
    N_("Additional altitude required to reach the next turn point."),
    e_Fin_AltDiff, // Fin AltD
    e_WP_AltDiff, // WP AltD
  },

  // 14
  {
    N_("Next Waypoint"),
    N_("Next"),
    N_("The name of the currently selected turn point. When this infobox is active, using the up/down cursor keys selects the next/previous waypoint in the task. (Touchscreen/PC only) Pressing the enter cursor key brings up the waypoint details."),
    e_TimeSinceTakeoff, // Time flt
    e_RH_Trend, // RH Trend
  },

  // 15
  {
    N_("Final Altitude Difference"),
    N_("Fin AltD"),
    N_("Arrival altitude at the final task turn point relative to the safety arrival altitude."),
    e_Fin_AltReq, // Fin AltR
    e_WP_AltReq, // WP AltR
  },

  // 16
  {
    N_("Final Altitude Required"),
    N_("Fin AltR"),
    N_("Additional altitude required to finish the task."),
    e_SpeedTaskAvg, // V Task Av
    e_Fin_AltDiff, // Fin AltD
  },

  // 17
  {
    N_("Speed Task Average"),
    N_("V Task Av"),
    N_("Average cross country speed while on current task, not compensated for altitude."),
    e_CC_SpeedInst, // V Task Inst
    e_Fin_AltReq, // Fin AltR
  },

  // 18
  {
    N_("Final Distance"),
    N_("Fin Dis"),
    N_("Distance to finish around remaining turn points."),
    e_AA_Time, // AA Time
    e_CC_Speed, // V Task Ach
  },

  // 19
  {
    N_("Final LD"),
    N_("Fin LD"),
    N_("The required glide ratio to finish the task, given by the distance to go divided by the height required to arrive at the safety arrival altitude. Negative values indicate a climb is necessary to finish. If the height required is close to zero, the displayed value is '---'. Note that this calculation may be optimistic because it reduces the height required to finish by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds."),
    e_Fin_GR, // Final GR
    e_LD_Cruise, // LD Cruise
  },

  // 20
  {
    N_("Terrain Elevation"),
    N_("H GND"),
    N_("This is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location."),
    e_H_Baro, // H Baro
    e_HeightAGL, // H AGL
  },

  // 21
  {
    N_("Thermal Average"),
    N_("TC Avg"),
    N_("Altitude gained/lost in the current thermal, divided by time spent thermaling."),
    e_Thermal_Gain, // TC Gain
    e_TL_Time, // TL Time
  },

  // 22
  {
    N_("Thermal Gain"),
    N_("TC Gain"),
    N_("The altitude gained/lost in the current thermal."),
    e_Climb_Avg, // TC All
    e_Thermal_Avg, // TC Avg
  },

  // 23
  {
    N_("Track"),
    N_("Track"),
    N_("Magnetic track reported by the GPS. (Touchscreen/PC only) If this infobox is active in simulation mode, pressing the up and down  arrows adjusts the track."),
    e_AirSpeed_Ext, // V IAS
    e_Speed_GPS, // V Gnd
  },

  // 24
  {
    N_("Vario"),
    N_("Vario"),
    N_("Instantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    e_VerticalSpeed_Netto, // Netto
    e_Climb_Avg, // TC All
  },

  // 25
  {
    N_("Wind Speed"),
    N_("Wind V"),
    N_("Wind speed estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust magnitude and left/right cursor keys to adjust bearing when the infobox is active. Pressing the enter cursor key saves the wind value as the initial value when XCSoar next starts."),
    e_WindBearing_Est, // Wind B
    e_Home_Temperature, // Max Temp
  },

  // 26
  {
    N_("Wind Bearing"),
    N_("Wind B"),
    N_("Wind bearing estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust bearing when the infobox is active."),
    e_Temperature, // OAT
    e_WindSpeed_Est, // Wind V
  },

  // 27
  {
    N_("AA Time"),
    N_("AA Time"),
    N_("Assigned Area Task time remaining. Goes red when time remaining has expired."),
    e_AA_TimeDiff, // AA dTime
    e_Fin_Distance, // Fin Dis
  },

  // 28
  {
    N_("AA Distance Max"),
    N_("AA Dmax"),
    N_("Assigned Area Task maximum distance possible for remainder of task."),
    e_AA_DistanceMin, // AA Dmin
    e_AA_TimeDiff, // AA dTime
  },

  // 29
  {
    N_("AA Distance Min"),
    N_("AA Dmin"),
    N_("Assigned Area Task minimum distance possible for remainder of task."),
    e_AA_SpeedMax, // AA Vmax
    e_AA_DistanceMax, // AA Dmax
  },

  // 30
  {
    N_("AA Speed Max"),
    N_("AA Vmax"),
    N_("Assigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time."),
    e_AA_SpeedMin, // AA Vmin
    e_AA_DistanceMin, // AA Dmin
  },

  // 31
  {
    N_("AA Speed Min"),
    N_("AA Vmin"),
    N_("Assigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time."),
    e_Fin_AA_Distance, // AA Dtgt
    e_AA_SpeedMax, // AA Vmax
  },

  // 32
  {
    N_("Airspeed IAS"),
    N_("V IAS"),
    N_("Indicated Airspeed reported by a supported external intelligent vario."),
    e_Load_G, // G load
    e_Track_GPS, // Track
  },

  // 33
  {
    N_("Height Baro"),
    N_("H Baro"),
    N_("This is the barometric altitude obtained from a GPS equipped with pressure sensor, or a supported external intelligent vario."),
    e_H_QFE, // QFE GPS
    e_H_Terrain, // H Gnd
  },

  // 34
  {
    N_("Speed MacCready"),
    N_("V MC"),
    N_("The MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent."),
    e_Climb_Perc, // % Climb
    e_MacCready, // MC
  },

  // 35
  {
    N_("Percentage climb"),
    N_("% Climb"),
    N_("Percentage of time spent in climb mode. These statistics are reset upon starting the task."),
    e_Act_Speed, // V Opt
    e_WP_Speed_MC, // V MC
  },

  // 36
  {
    N_("Time of flight"),
    N_("Time flt"),
    N_("Time elapsed since takeoff was detected."),
    e_TimeLocal, // Time local
    e_WP_Name, // Next
  },

  // 37
  {
    N_("G load"),
    N_("G"),
    N_("Magnitude of G loading reported by a supported external intelligent vario. This value is negative for pitch-down manoeuvres."),
    e_WP_BearingDiff, // Bearing D
    e_AirSpeed_Ext, // Track
  },

  // 38
  {
    N_("Next LD"),
    N_("WP LD"),
    N_("The required glide ratio to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival altitude. Negative values indicate a climb is necessary to reach the waypoint. If the height required is close to zero, the displayed value is '---'.   Note that this calculation may be optimistic because it reduces the height required to reach the waypoint by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds."),
    e_LD, // LD Vario
    e_Fin_GR, // Final GR
  },

  // 39
  {
    N_("Time local"),
    N_("Time loc"),
    N_("GPS time expressed in local time zone."),
    e_TimeUTC, // Time UTC
    e_TimeSinceTakeoff, // Time flt
  },

  // 40
  {
    N_("Time UTC"),
    N_("Time UTC"),
    N_("GPS time expressed in UTC."),
    e_Fin_Time, // Fin ETE
    e_TimeLocal, // Time local
  },

  // 41
  {
    N_("Task Time To Go"),
    N_("Fin ETE"),
    N_("Estimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle."),
    e_WP_Time, // WP ETE
    e_TimeUTC, // Time UTC
  },

  // 42
  {
    N_("Next Time To Go"),
    N_("WP ETE"),
    N_("Estimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    e_Fin_TimeLocal, // Fin ETA
    e_Fin_Time, // Fin ETE
  },

  // 43
  {
    N_("Speed Dolphin"),
    N_("V Opt"),
    N_("The instantaneous MacCready speed-to-fly, making use of Netto vario calculations to determine dolphin cruise speed in the glider's current bearing. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent. In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected). When Block mode speed to fly is selected, this infobox displays the MacCready speed."),
    e_MacCready, // MC
    e_Climb_Perc, // % Climb
  },

  // 44
  {
    N_("Netto Vario"),
    N_("Netto"),
    N_("Instantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate. Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates."),
    e_Thermal_30s, // TC 30s
    e_VerticalSpeed_GPS, // Vario
  },

  // 45
  {
    N_("Task Arrival Time"),
    N_("Fin ETA"),
    N_("Estimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle."),
    e_WP_TimeLocal, // WP ETA
    e_WP_Time, // WP ETE
  },

  // 46
  {
    N_("Next Arrival Time"),
    N_("WP ETA"),
    N_("Estimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    e_RH_Trend, // RH Trend
    e_Fin_TimeLocal, // Fin ETA
  },

  // 47
  {
    N_("Bearing Difference"),
    N_("Brng D"),
    N_("The difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector. GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present. Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint. This bearing takes into account the curvature of the Earth."),
    e_Speed, // V TAS
    e_Load_G, // G load
  },

  // 48
  {
    N_("Outside Air Temperature"),
    N_("OAT"),
    N_("Outside air temperature measured by a probe if supported by a connected intelligent variometer."),
    e_HumidityRel, // RelHum
    e_WindBearing_Est, // Wind B
  },

  // 49
  {
    N_("Relative Humidity"),
    N_("RelHum"),
    N_("Relative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer."),
    e_Home_Temperature, // MaxTemp
    e_Temperature, // OAT
  },

  // 50
  {
    N_("Forecast Temperature"),
    N_("MaxTemp"),
    N_("Forecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe. (Touchscreen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature."),
    e_WindSpeed_Est, // Wind V
    e_HumidityRel, // RelHum
  },

  // 51
  {
    N_("AA Distance Tgt"),
    N_("AA Dtgt"),
    N_("Assigned Area Task distance around target points for remainder of task."),
    e_AA_SpeedAvg, // AA Vtgt
    e_AA_SpeedMin, // AA Vmin
  },

  // 52
  {
    N_("AA Speed Tgt"),
    N_("AA Vtgt"),
    N_("Assigned Area Task average speed achievable around target points remaining in minimum AAT time."),
    e_Home_Distance, // Home Dis
    e_Fin_AA_Distance, // AA Dtgt
  },

  // 53
  {
    N_("L/D vario"),
    N_("L/D vario"),
    N_("Instantaneous glide ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer. Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---'."),
    e_LD_Avg, // LD Avg
    e_WP_LD, // Next LD
  },

  // 54
  {
    N_("Airspeed TAS"),
    N_("V TAS"),
    N_("True Airspeed reported by a supported external intelligent vario."),
    e_Bearing, // Bearing
    e_WP_BearingDiff, // Bearing Diff
  },

  // 55
  {
    N_("Team Code"),
    N_("TeamCode"),
    N_("The current Team code for this aircraft. Use this to report to other team members. The last team aircraft code entered is displayed underneath."),
    e_Team_Bearing, // Team Bearing
    e_Team_Range, // Team Range
  },

  // 56
  {
    N_("Team Bearing"),
    N_("Tm Brng"),
    N_("The bearing to the team aircraft location at the last team code report."),
    e_Team_BearingDiff, // Team Bearing Diff
    e_Team_Code, // Team Code
  },

  // 57
  {
    N_("Team Bearing Diff"),
    N_("Team Bd"),
    N_("The relative bearing to the team aircraft location at the last reported team code."),
    e_Team_Range, // Team Range
    e_Team_Bearing, // Team Bearing
  },

  // 58
  {
    N_("Team Range"),
    N_("Team Dis"),
    N_("The range to the team aircraft location at the last reported team code."),
    e_Team_Code, // Team Code
    e_Team_BearingDiff, // Team Bearing Diff
  },

  // 59
  {
    N_("Speed Task Instantaneous"),
    N_("V Tsk Ins"),
    N_("Instantaneous cross country speed while on current task, compensated for altitude. Equivalent to instantaneous Pirker cross-country speed."),
    e_CC_Speed, // V Task Ach
    e_SpeedTaskAvg, // V Task Av
  },

  // 60
  {
    N_("Distance Home"),
    N_("Home Dis"),
    N_("Distance to home waypoint (if defined)."),
    e_OC_Distance, // OLC
    e_AA_SpeedAvg, // AA Vtgt
  },

  // 61
  {
    N_("Speed Task Achieved"),
    N_("V Tsk Ach"),
    N_("Achieved cross country speed while on current task, compensated for altitude.  Equivalent to Pirker cross-country speed remaining."),
    e_Fin_Distance, // Fin Dis
    e_CC_SpeedInst, // V Task Inst
  },

  // 62
  {
    N_("AA Delta Time"),
    N_("AA dT"),
    N_("Difference between estimated task time and AAT minimum time. Colored red if negative (expected arrival too early), or blue if in sector and can turn now with estimated arrival time greater than AAT time plus 5 minutes."),
    e_AA_DistanceMax, // AA Dmax
    e_AA_Time, // AA Time
  },

  // 63
  {
    N_("Thermal All"),
    N_("TC All"),
    N_("Time-average climb rate in all thermals."),
    e_VerticalSpeed_GPS, // Vario
    e_Thermal_Gain, // TC Gain
  },

  // 64
  {
    N_("Task Req. Total Height Trend"),
    N_("RH Trend"),
    N_("Trend (or neg. of the variation) of the total required height to complete the task."),
    e_WP_Name, // Next
    e_WP_TimeLocal, // WP ETA
  },

  // 65
  {
#ifndef GNAV
    N_("Battery Percent"),
#else
    N_("Battery Voltage"),
#endif
    N_("Battery"),
    N_("Displays percentage of device battery remaining (where applicable) and status/voltage of external power supply."),
    e_CPU_Load, // CPU
    e_CPU_Load, // CPU
  },

  // 66
  {
    N_("Final GR"),
    N_("Fin GR"),
    N_("Geometric gradient to the arrival height above the final waypoint. This is not adjusted for total energy."),
    e_WP_LD, // Next LD
    e_Fin_LD, // Fin LD
  },

  // 67
  {
    N_("Alternate1 name"),
    N_("Altrn 1"),
    N_("Displays name and bearing to the best alternate landing location."),
    e_Alternate_2_Name, // Altern2 name
    e_Alternate_1_GR, // Altern1 GR
  },

  // 68
  {
    N_("Alternate2 name"),
    N_("Altrn 2"),
    N_("Displays name and bearing to the second alternate landing location."),
    e_Alternate_1_GR, // Altern1 GR
    e_Alternate_1_Name, // Altern1 name
  },

  // 69 
  {
    N_("Alternate1 GR"),
    N_("Altrn1 GR"),
    N_("Geometric gradient to the arrival height above the best alternate. This is not adjusted for total energy."),
    e_Alternate_1_Name, // Altern1 name
    e_Alternate_2_Name, // Altern2 name
  },

  // 70
  {
    N_("QFE GPS"),
    N_("QFE GPS"),
    N_("Automatic QFE. This altitude value is constantly reset to 0 on ground BEFORE taking off. After takeoff, it is no more reset automatically even if on ground. During flight you can change QFE with up and down keys. Bottom line shows QNH altitude. Changing QFE does not affect QNH altitude."),
    e_FlightLevel, // Flight Level
    e_H_Baro, // H Baro
  },

  // 71
  {
    N_("L/D Average"),
    N_("L/D Avg"),
    N_("The distance made in the configured period of time , divided by the altitude lost since then. Negative values are shown as ^^^ and indicate climbing cruise (height gain). Over 200 of LD the value is shown as +++ . You can configure the period of averaging in the Special config menu. Suggested values for this configuration are 60, 90 or 120: lower values will be closed to LD INST, and higher values will be closed to LD Cruise. Notice that the distance is NOT the straight line between your old and current position: it's exactly the distance you have made even in a zigzag glide. This value is not calculated while circling. "),
    e_LD_Instantaneous, // LD Inst
    e_LD, // LD Vario
  },

  // 72
  {
    N_("Experimental1"),
    N_("Exp1"),
    NULL,
    e_Experimental2, // Exp2
    e_Experimental2, // Exp2
  },

  // 73
  {
    N_("Online Contest Distance"),
    N_("OLC"),
    N_("Instantaneous evaluation of the flown distance according to the configured Online-Contest rule set."),
    e_WP_Distance, // WP Dist
    e_Home_Distance, // Home Dis
  },

  // 74
  {
    N_("Experimental2"),
    N_("Exp2"),
    NULL,
    e_Experimental1, // Exp1
    e_Experimental1, // Exp1
  },

  // 75
  {
    N_("CPU Load"),
    N_("CPU"),
    N_("CPU load consumed by XCSoar averaged over 5 seconds."),
    e_Battery, // Battery
    e_Battery, // Battery
  },

  // 76
  {
    N_("Next Altitude Arrival"),
    N_("WP AltA"),
    N_("Absolute arrival altitude at the next waypoint in final glide."),
    e_WP_AltReq, // WP AltR
    e_WP_AltDiff, // WP AltD
  },

  // 77
  {
    N_("Free RAM"),
    N_("Free RAM"),
    N_("Free RAM as reported by OS."),
    75, // CPU Load
    75, // CPU Load
  },

  // 78
  {
    N_("Flight Level"),
    N_("Flight Level"),
    N_("Pressure Altitude given as Flight Level. Only available if barometric altitude available and correct QNH set."),
    0, // H GPS
    70, // QFE GPS
  },

  // 79
  {
    N_("Barogram"),
    N_("Barogram"),
    N_("Trace of altitude during flight"),
    79, // H GPS
    79, // QFE GPS
  },

  // 80
  {
    N_("Vario trace"),
    N_("Vario"),
    N_("Trace of vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    79, // H GPS
    79, // QFE GPS
  },

  // 81
  {
    N_("Netto vario trace"),
    N_("Netto"),
    N_("Trace of vertical speed of air-mass, equal to vario value less the glider's estimated sink rate."),
    79, // H GPS
    79, // QFE GPS
  },
  
  // 82
  {
    N_("Thermal circling trace"),
    N_("TC Circling"),
    N_("Trace of average climb rate each turn in circling, based of the reported GPS altitude, or vario if available."),
    79, // H GPS
    79, // QFE GPS
  },

  // 83
  {
    N_("Climb band"),
    N_("Climb band"),
    N_("Graph of average circling climb rate (horizontal axis) as a function of height (vertical axis)."),
    79, // H GPS
    79, // QFE GPS
  },

  // 84
  {
    N_("Time Under Max Start Height"),
    N_("Start Height"),
    N_("The contiguous period the ship has been below the task start max height."),
    e_TaskMaxHeightTime,
    e_TaskMaxHeightTime,
  },

};

InfoBoxContent*
InfoBoxFactory::Create(unsigned InfoBoxType)
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
  case e_TaskMaxHeightTime:
    return new InfoBoxContentTaskTimeUnderMaxHeight();
  }

  return NULL;
}
