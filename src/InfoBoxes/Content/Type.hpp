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

#ifndef XCSOAR_INFOBOX_TYPE_HPP
#define XCSOAR_INFOBOX_TYPE_HPP

namespace InfoBoxFactory
{
  enum Type {
    /* 0..9 */
    e_HeightGPS, /* This is the height above mean sea level reported by the GPS. Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn */
    e_HeightAGL, /* This is the navigation altitude minus the terrain height obtained from the terrain file. The value is coloured red when the glider is below the terrain safety clearance height */
    e_Thermal_30s, /* A 30 second rolling average climb rate based of the reported GPS altitude, or vario if available */
    e_Bearing, /* True bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector */
    e_GR_Instantaneous, /* Instantaneous glide ratio over ground, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds. Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---' */
    e_GR_Cruise, /* The distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal. Negative values indicate climbing cruise (height gain since leaving the last thermal). If the vertical speed is close to zero, the displayed value is '---' */
    e_Speed_GPS, /* Ground speed measured by the GPS. If this infobox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider */
    e_TL_Avg, /* Total altitude gain/loss in the last thermal divided by the time spent circling */
    e_TL_Gain, /* Total altitude gain/loss in the last thermal */
    e_TL_Time, /* Time spent circling in the last thermal */
    /* 10..19 */
    e_MacCready, /* The current MacCready setting. This infobox also shows whether MacCready is manual or auto. (Touchscreen/PC only) Also used to adjust the MacCready Setting if the infobox is active, by using the up/down cursor keys */
    e_WP_Distance, /* The distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector */
    e_WP_AltDiff, /* Next Altitude Difference - Arrival altitude at the next waypoint relative to the safety arrival height */
    e_WP_AltReq, /* Additional altitude required to reach the next turn point */
    e_WP_Name, /* The name of the currently selected turn point. When this infobox is active, using the up/down cursor keys selects the next/previous waypoint in the task. (Touchscreen/PC only) Pressing the enter cursor key brings up the waypoint details */
    e_Fin_AltDiff, /* Arrival altitude at the final task turn point relative to the safety arrival height */
    e_Fin_AltReq, /* Additional altitude required to finish the task */
    e_SpeedTaskAvg, /* Average cross country speed while on current task, compensated for altitude */
    e_Fin_Distance, /* Distance to finish around remaining turn points */
    e_Fin_GR_TE, /* Deprecated */
    /* 20..29 */
    e_H_Terrain, /* This is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location */
    e_Thermal_Avg, /* Altitude gained/lost in the current thermal, divided by time spent thermaling */
    e_Thermal_Gain, /* The altitude gained/lost in the current thermal */
    e_Track_GPS, /* Magnetic track reported by the GPS. (Touchscreen/PC only) If this infobox is active in simulation mode, pressing the up and down  arrows adjusts the track */
    e_VerticalSpeed_GPS, /* Instantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one */
    e_WindSpeed_Est, /* Wind speed estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust magnitude and left/right cursor keys to adjust bearing when the infobox is active. Pressing the enter cursor key saves the wind value as the initial value when XCSoar next starts */
    e_WindBearing_Est, /* Wind bearing estimated by XCSoar. (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust bearing when the infobox is active */
    e_AA_Time, /* Assigned Area Task time remaining. Goes red when time remaining has expired */
    e_AA_DistanceMax, /* Assigned Area Task maximum distance possible for remainder of task */
    e_AA_DistanceMin, /* Assigned Area Task minimum distance possible for remainder of task */
    /* 30..39 */
    e_AA_SpeedMax, /* Assigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time */
    e_AA_SpeedMin, /* Assigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time */
    e_AirSpeed_Ext, /* Indicated Airspeed reported by a supported external intelligent vario */
    e_H_Baro, /* This is the barometric altitude obtained from a GPS equipped with pressure sensor, or a supported external intelligent vario */
    e_WP_Speed_MC, /* The MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent */
    e_Climb_Perc, /* Percentage of time spent in climb mode. These statistics are reset upon starting the task */
    e_TimeSinceTakeoff, /* Time elapsed since takeoff was detected */
    e_Load_G, /* Magnitude of G loading reported by a supported external intelligent vario. This value is negative for pitch-down manoeuvres */
    e_WP_GR, /* The required glide ratio over ground to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival height. Negative values indicate a climb is necessary to reach the waypoint. If the height required is close to zero, the displayed value is '---'.   Note that this calculation may be optimistic because it reduces the height required to reach the waypoint by the excess energy height of the glider if its true airspeed is greater than the MacCready and best LD speeds */
    e_TimeLocal, /* GPS time expressed in local time zone */
    /* 40..49 */
    e_TimeUTC, /* GPS time expressed in UTC */
    e_Fin_Time, /* Estimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle */
    e_WP_Time, /* Estimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle */
    e_Act_Speed, /* The instantaneous MacCready speed-to-fly, making use of Netto vario calculations to determine dolphin cruise speed in the glider's current bearing. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude. In final glide mode, this speed-to-fly is calculated for descent. In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected). When Block mode speed to fly is selected, this infobox displays the MacCready speed */
    e_VerticalSpeed_Netto, /* Instantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate. Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates */
    e_Fin_TimeLocal, /* Estimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle */
    e_WP_TimeLocal, /* Estimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle */
    e_WP_BearingDiff, /* The difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector. GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present. Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint. This bearing takes into account the curvature of the Earth */
    e_Temperature, /* Outside air temperature measured by a probe if supported by a connected intelligent variometer */
    e_HumidityRel, /* Relative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer */
    /* 50..59 */
    e_Home_Temperature, /* Forecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe. (Touchscreen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature */
    e_Fin_AA_Distance, /* Assigned Area Task distance around target points for remainder of task */
    e_AA_SpeedAvg, /* Assigned Area Task average speed achievable around target points remaining in minimum AAT time */
    e_LD, /* Instantaneous lift/drag ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer. Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---' */
    e_Speed, /* True Airspeed reported by a supported external intelligent vario */
    e_Team_Code, /* The current Team code for this aircraft. Use this to report to other team members. The last team aircraft code entered is displayed underneath */
    e_Team_Bearing, /* The bearing to the team aircraft location at the last team code report */
    e_Team_BearingDiff, /* The relative bearing to the team aircraft location at the last reported team code */
    e_Team_Range, /* The range to the team aircraft location at the last reported team code */
    e_CC_SpeedInst, /* Instantaneous cross country speed while on current task, compensated for altitude */
    /* 60..69 */
    e_Home_Distance, /* Distance to home waypoint (if defined) */
    e_CC_Speed, /* Achieved cross country speed while on current task, compensated for altitude */
    e_AA_TimeDiff, /* Difference between estimated task time and AAT minimum time. Colored red if negative (expected arrival too early), or blue if in sector and can turn now with estimated arrival time greater than AAT time plus 5 minutes */
    e_Climb_Avg, /* Time-average climb rate in all thermals */
    e_RH_Trend, /* Task Req. Total Height Trend */
    e_Battery, /* Displays percentage of device battery remaining (where applicable) and status/voltage of external power supply */
    e_Fin_GR, /* Geometric gradient to the arrival height above the final waypoint. This is not adjusted for total energy */
    e_Alternate_1_Name, /* Displays name and bearing to the best alternate landing location */
    e_Alternate_2_Name, /* Displays name and bearing to the second alternate landing location */
    e_Alternate_1_GR, /* Geometric gradient to the arrival height above the best alternate. This is not adjusted for total energy */
    /* 70..79 */
    e_H_QFE, /* Height on automatic QFE. This altitude value is constantly reset to 0 on ground BEFORE taking off. After takeoff, it is no more reset automatically even if on ground. During flight you can change QFE with up and down keys. Bottom line shows QNH altitude. Changing QFE does not affect QNH altitude */
    e_GR_Avg, /* The distance made in the configured period of time divided by the altitude lost since then. */
    e_Experimental1, /* Experimental1 */
    e_OC_Distance, /* Online Contest Distance */
    e_Experimental2, /* Experimental2 */
    e_CPU_Load, /* CPU load consumed by XCSoar averaged over 5 seconds */
    e_WP_H, /* Absolute arrival altitude at the next waypoint in final glide */
    e_Free_RAM, /* Free RAM as reported by OS */
    e_FlightLevel, /* Flight Level, also known as pressure altitude */
    e_Barogram,
    /* 80..89 */
    e_Vario_spark,
    e_NettoVario_spark,
    e_CirclingAverage_spark,
    e_ThermalBand,
    e_TaskProgress,
    e_TaskMaxHeightTime, /* Time aircraft has been under the max start height */
    e_Fin_ETE_VMG,
    e_WP_ETE_VMG,
    e_Horizon,
    e_NearestAirspaceHorizontal,
    /* 90..99 */
    e_NearestAirspaceVertical,
    e_WP_MC0AltDiff,
    e_HeadWind,
    TerrainCollision,
    NavAltitude,
    NextLegEqThermal,
    HeadWindSimplified,
    CruiseEfficiency,
    WIND_ARROW,
    THERMAL_ASSISTANT,

    START_OPEN_TIME,
    START_OPEN_ARRIVAL_TIME,

    NEXT_RADIAL,
    ATC_RADIAL,

    TASK_SPEED_HOUR,
    WP_NOMINAL_DIST, /* The nominal distance to the currently selected waypoint. For AAT tasks, this is the distance to the origin of the AAT sector */

    CIRCLE_DIAMETER,

    e_NUM_TYPES /* Last item */
  };

  static constexpr Type NUM_TYPES = e_NUM_TYPES;
  static constexpr Type MIN_TYPE_VAL = (Type)0;
  static constexpr Type MAX_TYPE_VAL = (Type)(e_NUM_TYPES - 1);
}

#endif
