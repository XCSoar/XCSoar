/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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
#include "InfoBoxes/Content/Other.hpp"
#include "InfoBoxes/Content/Speed.hpp"
#include "InfoBoxes/Content/Task.hpp"
#include "InfoBoxes/Content/Team.hpp"
#include "InfoBoxes/Content/Thermal.hpp"
#include "InfoBoxes/Content/Time.hpp"
#include "InfoBoxes/Content/Weather.hpp"

#include "Language.hpp"

#include <stddef.h>

// Groups:
//   Altitude 0,1,20,33,70
//   Aircraft info 3,6,23,32,37,47,54
//   LD 4,5,19,66,38,53,71
//   Vario 2,7,8,9,21,22,63,24,44,64
//   Wind 25,26,48,49,50
//   MacCready 10,34,35,43
//   Nav 11,12,76,13,15,16,17,59,61,18,27,28,29,30,31,51,52,60,73
//   Waypoint 14,36,39,40,41,42,45,46
//   Team 55,56,57,58
//   Gadget 65,75
//   Alternates 67,68,69
//   Experimental 72,74
const InfoBoxFactory::InfoBoxMetaData InfoBoxFactory::MetaData[NUM_TYPES] = {
  // 0
  {
    N_("Height GPS"),
    N_("H GPS"),
    N_("This is the height above mean sea level reported by the GPS. Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn."),
    1, // H AGL
    70, // QFE MSL
  },

  // 1
  {
    N_("Height AGL"),
    N_("H AGL"),
    N_("This is the navigation altitude minus the terrain height obtained from the terrain file. The value is coloured red when the glider is below the terrain safety clearance height."),
    20, // H Gnd
    0, // H GPS
  },

  // 2
  {
    N_("Thermal last 30 sec"),
    N_("TC 30s"),
    N_("A 30 second rolling average climb rate based of the reported GPS altitude, or vario if available."),
    7, // TL Avg
    64, // D Vario
  },

  // 3
  {
    N_("Bearing"),
    N_("Bearing"),
    N_("True bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector."),
    6, // V Gnd
    54, // V TAS
  },

  // 4
  {
    N_("L/D instantaneous"),
    N_("L/D Inst"),
    N_("Instantaneous glide ratio, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds. Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---'."),
    5, // LD Cruise
    71, // LD Avg
  },

  // 5
  {
    N_("L/D cruise"),
    N_("L/D Cru"),
    N_("The distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal. Negative values indicate climbing cruise (height gain since leaving the last thermal). If the vertical speed is close to zero, the displayed value is '---'."),
    19, // Final LD
    4, // LD Inst
  },

  // 6
  {
    N_("Speed ground"),
    N_("V Gnd"),
    N_("Ground speed measured by the GPS. If this infobox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider."),
    23, // Track
    3, // Bearing
  },

  // 7
  {
    N_("Last Thermal Average"),
    N_("TL Avg"),
    N_("Total altitude gain/loss in the last thermal divided by the time spent circling."),
    8, // TL Gain
    2, // TC 30s
  },

  // 8
  {
    N_("Last Thermal Gain"),
    N_("TL Gain"),
    N_("Total altitude gain/loss in the last thermal."),
    9, // TL Time
    7, // TL Avg
  },

  // 9
  {
    N_("Last Thermal Time"),
    N_("TL Time"),
    N_("Time spent circling in the last thermal."),
    21, // TC Avg
    8, // TL Gain
  },

  // 10
  {
    N_("MacCready Setting"),
    N_("MacCready"),
    N_("The current MacCready setting. This infobox also shows whether MacCready is manual or auto. (Touchscreen/PC only) Also used to adjust the MacCready Setting if the infobox is active, by using the up/down cursor keys."),
    34, // V MC
    43, // V Opt
  },

  // 11
  {
    N_("Next Distance"),
    N_("WP Dist"),
    N_("The distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector."),
    12, // WP AltD
    73, // OLC
  },

  // 12
  {
    N_("Next Altitude Difference"),
    N_("WP AltD"),
    N_("Arrival altitude at the next waypoint relative to the safety arrival altitude."),
    76, // WP AltA
    11, // WP Dist
  },

  // 13
  {
    N_("Next Altitude Required"),
    N_("WP AltR"),
    N_("Additional altitude required to reach the next turn point."),
    15, // Fin AltD
    12, // WP AltD
  },

  // 14
  {
    N_("Next Waypoint"),
    N_("Next"),
    N_("The name of the currently selected turn point. When this infobox is active, using the up/down cursor keys selects the next/previous waypoint in the task. (Touchscreen/PC only) Pressing the enter cursor key brings up the waypoint details."),
    36, // Time flt
    46, // WP ETA
  },

  // 15
  {
    N_("Final Altitude Difference"),
    N_("Fin AltD"),
    N_("Arrival altitude at the final task turn point relative to the safety arrival altitude."),
    16, // Fin AltR
    13, // WP AltR
  },

  // 16
  {
    N_("Final Altitude Required"),
    N_("Fin AltR"),
    N_("Additional altitude required to finish the task."),
    17, // V Task Av
    15, // Fin AltD
  },

  // 17
  {
    N_("Speed Task Average"),
    N_("V Task Av"),
    N_("Average cross country speed while on current task, compensated for altitude."),
    59, // V Task Inst
    16, // Fin AltR
  },

  // 18
  {
    N_("Final Distance"),
    N_("Fin Dis"),
    N_("Distance to finish around remaining turn points."),
    27, // AA Time
    61, // V Task Ach
  },

  // 19
  {
    N_("Final LD"),
    N_("Fin LD"),
    N_("The required glide ratio to finish the task, given by the distance to go divided by the height required to arrive at the safety arrival altitude. Negative values indicate a climb is necessary to finish. If the height required is close to zero, the displayed value is '---'. Note that this calculation may be optimistic because it reduces the height required to finish by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds."),
    66, // Final GR
    5, // LD Cruise
  },

  // 20
  {
    N_("Terrain Elevation"),
    N_("H GND"),
    N_("This is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location."),
    33, // H Baro
    1, // H AGL
  },

  // 21
  {
    N_("Thermal Average"),
    N_("TC Avg"),
    N_("Altitude gained/lost in the current thermal, divided by time spent thermaling."),
    22, // TC Gain
    9, // TL Time
  },

  // 22
  {
    N_("Thermal Gain"),
    N_("TC Gain"),
    N_("The altitude gained/lost in the current thermal."),
    63, // TC All
    21, // TC Avg
  },

  // 23
  {
    N_("Track"),
    N_("Track"),
    N_("Magnetic track reported by the GPS. (Touchscreen/PC only) If this infobox is active in simulation mode, pressing the up and down  arrows adjusts the track."),
    32, // V IAS
    6, // V Gnd
  },

  // 24
  {
    N_("Vario"),
    N_("Vario"),
    N_("Instantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."),
    44, // Netto
    63, // TC All
  },

  // 25
  {
    N_("Wind Speed"),
    N_("Wind V"),
    N_("Wind speed estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust magnitude and left/right cursor keys to adjust bearing when the infobox is active. Pressing the enter cursor key saves the wind value as the initial value when XCSoar next starts."),
    26, // Wind B
    50, // Max Temp
  },

  // 26
  {
    N_("Wind Bearing"),
    N_("Wind B"),
    N_("Wind bearing estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust bearing when the infobox is active."),
    48, // OAT
    25, // Wind V
  },

  // 27
  {
    N_("AA Time"),
    N_("AA Time"),
    N_("Assigned Area Task time remaining. Goes red when time remaining has expired."),
    62, // AA dTime
    18, // Fin Dis
  },

  // 28
  {
    N_("AA Distance Max"),
    N_("AA Dmax"),
    N_("Assigned Area Task maximum distance possible for remainder of task."),
    29, // AA Dmin
    62, // AA dTime
  },

  // 29
  {
    N_("AA Distance Min"),
    N_("AA Dmin"),
    N_("Assigned Area Task minimum distance possible for remainder of task."),
    30, // AA Vmax
    28, // AA Dmax
  },

  // 30
  {
    N_("AA Speed Max"),
    N_("AA Vmax"),
    N_("Assigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time."),
    31, // AA Vmin
    29, // AA Dmin
  },

  // 31
  {
    N_("AA Speed Min"),
    N_("AA Vmin"),
    N_("Assigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time."),
    51, // AA Dtgt
    30, // AA Vmax
  },

  // 32
  {
    N_("Airspeed IAS"),
    N_("V IAS"),
    N_("Indicated Airspeed reported by a supported external intelligent vario."),
    37, // G load
    23, // Track
  },

  // 33
  {
    N_("Pressure Altitude"),
    N_("H Baro"),
    N_("This is the barometric altitude obtained from a GPS equipped with pressure sensor, or a supported external intelligent vario."),
    70, // QFE GPS
    20, // H Gnd
  },

  // 34
  {
    N_("Speed MacCready"),
    N_("V Mc"),
    N_("The MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent."),
    35, // % Climb
    10, // MC
  },

  // 35
  {
    N_("Percentage climb"),
    N_("% Climb"),
    N_("Percentage of time spent in climb mode. These statistics are reset upon starting the task."),
    43, // V Opt
    34, // V MC
  },

  // 36
  {
    N_("Time of flight"),
    N_("Time flt"),
    N_("Time elapsed since takeoff was detected."),
    39, // Time local
    14, // Next
  },

  // 37
  {
    N_("G load"),
    N_("G"),
    N_("Magnitude of G loading reported by a supported external intelligent vario. This value is negative for pitch-down manoeuvres."),
    47, // Bearing D
    32, // Track
  },

  // 38
  {
    N_("Next LD"),
    N_("WP LD"),
    N_("The required glide ratio to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival altitude. Negative values indicate a climb is necessary to reach the waypoint. If the height required is close to zero, the displayed value is '---'.   Note that this calculation may be optimistic because it reduces the height required to reach the waypoint by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds."),
    53, // LD Vario
    66, // Final GR
  },

  // 39
  {
    N_("Time local"),
    N_("Time loc"),
    N_("GPS time expressed in local time zone."),
    40, // Time UTC
    36, // Time flt
  },

  // 40
  {
    N_("Time UTC"),
    N_("Time UTC"),
    N_("GPS time expressed in UTC."),
    41, // Fin ETE
    39, // Time local
  },

  // 41
  {
    N_("Task Time To Go"),
    N_("Fin ETE"),
    N_("Estimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle."),
    42, // WP ETE
    40, // Time UTC
  },

  // 42
  {
    N_("Next Time To Go"),
    N_("WP ETE"),
    N_("Estimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    45, // Fin ETA
    41, // Fin ETE
  },

  // 43
  {
    N_("Speed Dolphin"),
    N_("V Opt"),
    N_("The instantaneous MacCready speed-to-fly, making use of Netto vario calculations to determine dolphin cruise speed in the glider's current bearing. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent. In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected). When Block mode speed to fly is selected, this infobox displays the MacCready speed."),
    10, // MC
    35, // % Climb
  },

  // 44
  {
    N_("Netto Vario"),
    N_("Netto"),
    N_("Instantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate. Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates."),
    64, // D Vario
    24, // Vario
  },

  // 45
  {
    N_("Task Arrival Time"),
    N_("Fin ETA"),
    N_("Estimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle."),
    46, // WP ETA
    42, // WP ETE
  },

  // 46
  {
    N_("Next Arrival Time"),
    N_("WP ETA"),
    N_("Estimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle."),
    14, // Next
    45, // Fin ETA
  },

  // 47
  {
    N_("Bearing Difference"),
    N_("Brng D"),
    N_("The difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector. GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present. Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint. This bearing takes into account the curvature of the Earth."),
    54, // V TAS
    37, // G load
  },

  // 48
  {
    N_("Outside Air Temperature"),
    N_("OAT"),
    N_("Outside air temperature measured by a probe if supported by a connected intelligent variometer."),
    49, // RelHum
    26, // Wind B
  },

  // 49
  {
    N_("Relative Humidity"),
    N_("RelHum"),
    N_("Relative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer."),
    50, // MaxTemp
    48, // OAT
  },

  // 50
  {
    N_("Forecast Temperature"),
    N_("MaxTemp"),
    N_("Forecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe. (Touchscreen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature."),
    25, // Wind V
    49, // RelHum
  },

  // 51
  {
    N_("AA Distance Tgt"),
    N_("AA Dtgt"),
    N_("Assigned Area Task distance around target points for remainder of task."),
    52, // AA Vtgt
    31, // AA Vmin
  },

  // 52
  {
    N_("AA Speed Tgt"),
    N_("AA Vtgt"),
    N_("Assigned Area Task average speed achievable around target points remaining in minimum AAT time."),
    60, // Home Dis
    51, // AA Dtgt
  },

  // 53
  {
    N_("L/D vario"),
    N_("L/D vario"),
    N_("Instantaneous glide ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer. Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---'."),
    71, // LD Avg
    38, // Next LD
  },

  // 54
  {
    N_("Airspeed TAS"),
    N_("V TAS"),
    N_("True Airspeed reported by a supported external intelligent vario."),
    3, // Bearing
    47, // Bearing Diff
  },

  // 55
  {
    N_("Team Code"),
    N_("TeamCode"),
    N_("The current Team code for this aircraft. Use this to report to other team members. The last team aircraft code entered is displayed underneath."),
    56, // Team Bearing
    58, // Team Range
  },

  // 56
  {
    N_("Team Bearing"),
    N_("Tm Brng"),
    N_("The bearing to the team aircraft location at the last team code report."),
    57, // Team Bearing Diff
    55, // Team Code
  },

  // 57
  {
    N_("Team Bearing Diff"),
    N_("Team Bd"),
    N_("The relative bearing to the team aircraft location at the last reported team code."),
    58, // Team Range
    56, // Team Bearing
  },

  // 58
  {
    N_("Team Range"),
    N_("Team Dis"),
    N_("The range to the team aircraft location at the last reported team code."),
    55, // Team Code
    57, // Team Bearing Diff
  },

  // 59
  {
    N_("Speed Task Instantaneous"),
    N_("V Tsk Ins"),
    N_("Instantaneous cross country speed while on current task, compensated for altitude."),
    61, // V Task Ach
    17, // V Task Av
  },

  // 60
  {
    N_("Distance Home"),
    N_("Home Dis"),
    N_("Distance to home waypoint (if defined)."),
    73, // OLC
    52, // AA Vtgt
  },

  // 61
  {
    N_("Speed Task Achieved"),
    N_("V Tsk Ach"),
    N_("Achieved cross country speed while on current task, compensated for altitude."),
    18, // Fin Dis
    59, // V Task Inst
  },

  // 62
  {
    N_("AA Delta Time"),
    N_("AA dT"),
    N_("Difference between estimated task time and AAT minimum time. Colored red if negative (expected arrival too early), or blue if in sector and can turn now with estimated arrival time greater than AAT time plus 5 minutes."),
    28, // AA Dmax
    27, // AA Time
  },

  // 63
  {
    N_("Thermal All"),
    N_("TC All"),
    N_("Time-average climb rate in all thermals."),
    24, // Vario
    22, // TC Gain
  },

  // 64
  {
    N_("Distance Vario"),
    N_("D Vario"),
    NULL,
    2, // TC 30s
    44, // Netto
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
    75, // CPU
    75, // CPU
  },

  // 66
  {
    N_("Final GR"),
    N_("Fin GR"),
    N_("Geometric gradient to the arrival height above the final waypoint. This is not adjusted for total energy."),
    38, // Next LD
    19, // Fin LD
  },

  // 67
  {
    N_("Alternate1 name"),
    N_("Altrn 1"),
    N_("Displays name and bearing to the best alternate landing location."),
    68, // Altern2 name
    69, // Altern1 GR
  },

  // 68
  {
    N_("Alternate2 name"),
    N_("Altrn 2"),
    N_("Displays name and bearing to the second alternate landing location."),
    69, // Altern1 GR
    67, // Altern1 name
  },

  // 69 
  {
    N_("Alternate1 GR"),
    N_("Altrn1 GR"),
    N_("Geometric gradient to the arrival height above the best alternate. This is not adjusted for total energy."),
    67, // Altern1 name
    68, // Altern2 name
  },

  // 70
  {
    N_("QFE GPS"),
    N_("QFE GPS"),
    N_("Automatic QFE. This altitude value is constantly reset to 0 on ground BEFORE taking off. After takeoff, it is no more reset automatically even if on ground. During flight you can change QFE with up and down keys. Bottom line shows QNH altitude. Changing QFE does not affect QNH altitude."),
    0, // H MSL
    33, // H Baro
  },

  // 71
  {
    N_("L/D Average"),
    N_("L/D Avg"),
    N_("The distance made in the configured period of time , divided by the altitude lost since then. Negative values are shown as ^^^ and indicate climbing cruise (height gain). Over 200 of LD the value is shown as +++ . You can configure the period of averaging in the Special config menu. Suggested values for this configuration are 60, 90 or 120: lower values will be closed to LD INST, and higher values will be closed to LD Cruise. Notice that the distance is NOT the straight line between your old and current position: it's exactly the distance you have made even in a zigzag glide. This value is not calculated while circling. "),
    4, // LD Inst
    53, // LD Vario
  },

  // 72
  {
    N_("Experimental1"),
    N_("Exp1"),
    NULL,
    74, // Exp2
    74, // Exp2
  },

  // 73
  {
    N_("Online Contest Distance"),
    N_("OLC"),
    NULL,
    11, // WP Dist
    60, // Home Dis
  },

  // 74
  {
    N_("Experimental2"),
    N_("Exp2"),
    NULL,
    72, // Exp1
    72, // Exp1
  },

  // 75
  {
    N_("CPU Load"),
    N_("CPU"),
    N_("CPU load consumed by XCSoar averaged over 5 seconds."),
    65, // Battery
    65, // Battery
  },

  // 76
  {
    N_("Next Altitude Arrival"),
    N_("WP AltA"),
    N_("Absolute arrival altitude at the next waypoint in final glide."),
    13, // WP AltR
    12, // WP AltD
  },
};

InfoBoxContent*
InfoBoxFactory::Create(unsigned InfoBoxType)
{
  switch (InfoBoxType) {
  case 0:
    return new InfoBoxContentAltitudeGPS();
  case 1:
    return new InfoBoxContentAltitudeAGL();
  case 2:
    return new InfoBoxContentThermal30s();
  case 3:
    return new InfoBoxContentBearing();
  case 4:
    return new InfoBoxContentLDInstant();
  case 5:
    return new InfoBoxContentLDCruise();
  case 6:
    return new InfoBoxContentSpeedGround();
  case 7:
    return new InfoBoxContentThermalLastAvg();
  case 8:
    return new InfoBoxContentThermalLastGain();
  case 9:
    return new InfoBoxContentThermalLastTime();
  case 10:
    return new InfoBoxContentMacCready();
  case 11:
    return new InfoBoxContentNextDistance();
  case 12:
    return new InfoBoxContentNextAltitudeDiff();
  case 13:
    return new InfoBoxContentNextAltitudeRequire();
  case 14:
    return new InfoBoxContentNextWaypoint();
  case 15:
    return new InfoBoxContentFinalAltitudeDiff();
  case 16:
    return new InfoBoxContentFinalAltitudeRequire();
  case 17:
    return new InfoBoxContentTaskSpeed();
  case 18:
    return new InfoBoxContentFinalDistance();
  case 19:
    return new InfoBoxContentFinalLD();
  case 20:
    return new InfoBoxContentTerrainHeight();
  case 21:
    return new InfoBoxContentThermalAvg();
  case 22:
    return new InfoBoxContentThermalGain();
  case 23:
    return new InfoBoxContentTrack();
  case 24:
    return new InfoBoxContentVario();
  case 25:
    return new InfoBoxContentWindSpeed();
  case 26:
    return new InfoBoxContentWindBearing();
  case 27:
    return new InfoBoxContentTaskAATime();
  case 28:
    return new InfoBoxContentTaskAADistanceMax();
  case 29:
    return new InfoBoxContentTaskAADistanceMin();
  case 30:
    return new InfoBoxContentTaskAASpeedMax();
  case 31:
    return new InfoBoxContentTaskAASpeedMin();
  case 32:
    return new InfoBoxContentSpeedIndicated();
  case 33:
    return new InfoBoxContentAltitudeBaro();
  case 34:
    return new InfoBoxContentSpeedMacCready();
  case 35:
    return new InfoBoxContentThermalRatio();
  case 36:
    return new InfoBoxContentTimeFlight();
  case 37:
    return new InfoBoxContentGLoad();
  case 38:
    return new InfoBoxContentNextLD();
  case 39:
    return new InfoBoxContentTimeLocal();
  case 40:
    return new InfoBoxContentTimeUTC();
  case 41:
    return new InfoBoxContentFinalETE();
  case 42:
    return new InfoBoxContentNextETE();
  case 43:
    return new InfoBoxContentSpeedDolphin();
  case 44:
    return new InfoBoxContentVarioNetto();
  case 45:
    return new InfoBoxContentFinalETA();
  case 46:
    return new InfoBoxContentNextETA();
  case 47:
    return new InfoBoxContentBearingDiff();
  case 48:
    return new InfoBoxContentTemperature();
  case 49:
    return new InfoBoxContentHumidity();
  case 50:
    return new InfoBoxContentTemperatureForecast();
  case 51:
    return new InfoBoxContentTaskAADistance();
  case 52:
    return new InfoBoxContentTaskAASpeed();
  case 53:
    return new InfoBoxContentLDVario();
  case 54:
    return new InfoBoxContentSpeed();
  case 55:
    return new InfoBoxContentTeamCode();
  case 56:
    return new InfoBoxContentTeamBearing();
  case 57:
    return new InfoBoxContentTeamBearingDiff();
  case 58:
    return new InfoBoxContentTeamDistance();
  case 59:
    return new InfoBoxContentTaskSpeedInstant();
  case 60:
    return new InfoBoxContentHomeDistance();
  case 61:
    return new InfoBoxContentTaskSpeedAchieved();
  case 62:
    return new InfoBoxContentTaskAATimeDelta();
  case 63:
    return new InfoBoxContentThermalAllAvg();
  case 64:
    return new InfoBoxContentVarioDistance();
  case 65:
    return new InfoBoxContentBattery();
  case 66:
    return new InfoBoxContentFinalGR();
  case 67:
    return new InfoBoxContentAlternateName(0);
  case 68:
    return new InfoBoxContentAlternateName(1);
  case 69:
    return new InfoBoxContentAlternateGR(0);
  case 70:
    return new InfoBoxContentAltitudeQFE();
  case 71:
    return new InfoBoxContentLDAvg();
  case 72:
    return new InfoBoxContentExperimental1();
  case 73:
    return new InfoBoxContentOLC();
  case 74:
    return new InfoBoxContentExperimental2();
  case 75:
    return new InfoBoxContentCPULoad();
  case 76:
    return new InfoBoxContentNextAltitudeArrival();
  }

  return NULL;
}
