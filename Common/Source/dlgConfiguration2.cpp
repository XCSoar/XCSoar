/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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


#include "StdAfx.h"
#include <aygshell.h>

#include "XCSoar.h"
#include "Statistics.h"
#include "externs.h"
#include "dlgTools.h"
#include "Utils.h"
#include "InfoBoxLayout.h"
#include "Waypointparser.h"

extern int config_page;

void OnInfoBoxHelp(WindowControl * Sender){
  WndProperty *wp = (WndProperty*)Sender;
  int type = wp->GetDataField()->GetAsInteger();
  TCHAR caption[100];
  TCHAR mode[100];
  switch (config_page) {
  case 14:
    _tcscpy(mode,gettext(TEXT("circling")));
    break;
  case 15:
    _tcscpy(mode,gettext(TEXT("cruise")));
    break;
  case 16:
    _tcscpy(mode,gettext(TEXT("final glide")));
    break;
  case 17:
    _tcscpy(mode,gettext(TEXT("auxiliary")));
    break;
  default:
    _tcscpy(mode,gettext(TEXT("Error")));
    return;
  }
  _stprintf(caption, TEXT("InfoBox %s in %s mode"),
	    wp->GetCaption(), mode);
  switch(type) {
  case 0:
    dlgHelpShowModal(caption, TEXT("[Height GPS]\r\nThis is the height above mean sea level reported by the GPS.\r\n Touchscreen/PC only: in simulation mode, this value is adjustable with the up/down arrow keys and the right/left arrow keys also cause the glider to turn."));
    break;
  case 1:
    dlgHelpShowModal(caption, TEXT("[Height AGL]\r\nThis is the navigation altitude minus the terrain height obtained from the terrain file.  The value is coloured red when the glider is below the terrain safety clearance height."));
    break;
  case 2:
    dlgHelpShowModal(caption, TEXT("[Thermal last 30 sec]\r\nA 30 second rolling average climb rate based of the reported GPS altitude, or vario if available."));
    break;
  case 3:
    dlgHelpShowModal(caption, TEXT("[Bearing]\r\nTrue bearing of the next waypoint.  For AAT tasks, this is the true bearing to the target within the AAT sector."));
    break;
  case 4:
    dlgHelpShowModal(caption, TEXT("[L/D instantaneous]\r\nInstantaneous glide ratio, given by the ground speed divided by the vertical speed (GPS speed) over the last 20 seconds.  Negative values indicate climbing cruise. If the vertical speed is close to zero, the displayed value is '---'."));
    break;
  case 5:
    dlgHelpShowModal(caption, TEXT("[L/D cruise]\r\nThe distance from the top of the last thermal, divided by the altitude lost since the top of the last thermal.  Negative values indicate climbing cruise (height gain since leaving the last thermal).  If the vertical speed is close to zero, the displayed value is '---'."));
    break;
  case 6:
    dlgHelpShowModal(caption, TEXT("[Speed ground]\r\nGround speed measured by the GPS.  If this infobox is active in simulation mode, pressing the up and down arrows adjusts the speed, and left and right turn the glider."));
    break;
  case 7:
    dlgHelpShowModal(caption, TEXT("[Last Thermal Average]\r\nTotal altitude gain/loss in the last thermal divided by the time spent circling."));
    break;
  case 8:
    dlgHelpShowModal(caption, TEXT("[Last Thermal Gain]\r\nTotal altitude gain/loss in the last thermal."));
    break;
  case 9:
    dlgHelpShowModal(caption, TEXT("[Last Thermal Time]\r\nTime spent circling in the last thermal."));
    break;
  case 10:
    dlgHelpShowModal(caption, TEXT("[MacCready Setting]\r\nThe current MacCready setting.  This infobox also shows whether MacCready is manual or auto.  (Touchscreen/PC only) Also used to adjust the MacCready Setting if the infobox is active, by using the up/down cursor keys."));
    break;
  case 11:
    dlgHelpShowModal(caption, TEXT("[Next Distance]\r\nThe distance to the currently selected waypoint. For AAT tasks, this is the distance to the target within the AAT sector."));
    break;
  case 12:
    dlgHelpShowModal(caption, TEXT("[Next Altitude Difference]\r\nArrival altitude at the next waypoint relative to the safety arrival altitude."));
    break;
  case 13:
    dlgHelpShowModal(caption, TEXT("[Next Altitude Required]\r\nAltitude required to reach the next turn point."));
    break;
  case 14:
    dlgHelpShowModal(caption, TEXT("[Next Waypoint]\r\n The name of the currently selected turn point.  When this infobox is active, using the up/down cursor keys selects the next/previous waypoint in the task.  (Touchscreen/PC only) Pressing the enter cursor key brings up the waypoint details."));
    break;
  case 15:
    dlgHelpShowModal(caption, TEXT("[Final Altitude Difference]\r\nArrival altitude at the final task turn point relative to the safety arrival altitude."));
    break;
  case 16:
    dlgHelpShowModal(caption, TEXT("[Final Altitude Required]\r\nAltitude required to finish the task."));
    break;
  case 17:
    dlgHelpShowModal(caption, TEXT("[Speed Task Average]\r\nAverage cross country speed while on current task, compensated for altitude."));
    break;
  case 18:
    dlgHelpShowModal(caption, TEXT("[Final Distance]\r\nDistance to finish around remaining turn points."));
    break;
  case 19:
    dlgHelpShowModal(caption, TEXT("[Final L/D]\r\nThe required glide ratio to finish the task, given by the distance to go divided by the height required to arrive at the safety arrival altitude.  Negative values indicate a climb is necessary to finish. If the height required is close to zero, the displayed value is '---'."));
    break;
  case 20:
    dlgHelpShowModal(caption, TEXT("[Terrain Elevation]\r\nThis is the elevation of the terrain above mean sea level, obtained from the terrain file at the current GPS location."));
    break;
  case 21:
    dlgHelpShowModal(caption, TEXT("[Thermal Average]\r\nAltitude gained/lost in the current thermal, divided by time spent thermaling."));
    break;
  case 22:
    dlgHelpShowModal(caption, TEXT("[Thermal Gain]\r\nThe altitude gained/lost in the current thermal."));
    break;
  case 23:
    dlgHelpShowModal(caption, TEXT("[Track]\r\nMagnetic track reported by the GPS.  (Touchscreen/PC only) If this infobox is active in simulation mode, pressing the up and down  arrows adjusts the track."));
    break;
  case 24:
    dlgHelpShowModal(caption, TEXT("[Vario]\r\nInstantaneous vertical speed, as reported by the GPS, or the intelligent vario total energy vario value if connected to one."));
    break;
  case 25:
    dlgHelpShowModal(caption, TEXT("[Wind Speed]\r\nWind speed estimated by XCSoar.  (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust magnitude and left/right cursor keys to adjust bearing when the infobox is active.  Pressing the enter cursor key saves the wind value as the initial value when XCSoar next starts."));
    break;
  case 26:
    dlgHelpShowModal(caption, TEXT("[Wind Bearing]\r\nWind bearing estimated by XCSoar.  (Touchscreen/PC only) Manual adjustment is possible by pressing the up/down cursor keys to adjust bearing when the infobox is active."));
    break;
  case 27:
    dlgHelpShowModal(caption, TEXT("[AA Time]\r\nAssigned Area Task time remaining.  Goes red when time remaining has expired."));
    break;
  case 28:
    dlgHelpShowModal(caption, TEXT("[AA Distance Max]\r\nAssigned Area Task maximum distance possible for remainder of task."));
    break;
  case 29:
    dlgHelpShowModal(caption, TEXT("[AA Distance Min]\r\nAssigned Area Task minimum distance possible for remainder of task."));
    break;
  case 30:
    dlgHelpShowModal(caption, TEXT("[AA Speed Max]\r\nAssigned Area Task average speed achievable if flying maximum possible distance remaining in minimum AAT time."));
    break;
  case 31:
    dlgHelpShowModal(caption, TEXT("[AA Speed Min]\r\nAssigned Area Task average speed achievable if flying minimum possible distance remaining in minimum AAT time."));
    break;
  case 32:
    dlgHelpShowModal(caption, TEXT("[Airspeed IAS]\r\nIndicated Airspeed reported by a supported external intelligent vario."));
    break;
  case 33:
    dlgHelpShowModal(caption, TEXT("[Pressure Altitude]\r\nThis is the barometric altitude obtained from a GPS equipped with pressure sensor, or a supported external intelligent vario."));
    break;
  case 34:
    dlgHelpShowModal(caption, TEXT("[Speed MacReady]\r\nThe MacCready speed-to-fly for optimal flight to the next waypoint. In cruise flight mode, this speed-to-fly is calculated for maintaining altitude.  In final glide mode, this speed-to-fly is calculated for descent."));
    break;
  case 35:
    dlgHelpShowModal(caption, TEXT("[Percentage climb]\r\nPercentage of time spent in climb mode.  These statistics are reset upon starting the task."));
    break;
  case 36:
    dlgHelpShowModal(caption, TEXT("[Time of flight]\r\nTime elapsed since takeoff was detected."));
    break;
  case 37:
    dlgHelpShowModal(caption, TEXT("[G load]\r\nMagnitude of G loading reported by a supported external intelligent vario.  This value is negative for pitch-down manoeuvres."));
    break;
  case 38:
    dlgHelpShowModal(caption, TEXT("[Next L/D]\r\nThe required glide ratio to reach the next waypoint, given by the distance to next waypoint divided by the height required to arrive at the safety arrival altitude.  Negative values indicate a climb is necessary to reach the waypoint.  If the height required is close to zero, the displayed value is '---'."));
    break;
  case 39:
    dlgHelpShowModal(caption, TEXT("[Time local]\r\nGPS time expressed in local time zone."));
    break;
  case 40:
    dlgHelpShowModal(caption, TEXT("[Time UTC]\r\nGPS time expressed in UTC."));
    break;
  case 41:
    dlgHelpShowModal(caption, TEXT("[Task Time To Go]\r\nEstimated time required to complete task, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 42:
    dlgHelpShowModal(caption, TEXT("[Next Time To Go]\r\nEstimated time required to reach next waypoint, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 43:
    dlgHelpShowModal(caption, TEXT("[Speed Dolphin]\r\nThe instantaneous MacCready speed-to-fly, making use of Netto vario calculations to determine dolphin cruise speed in the glider's current bearing.  In cruise flight mode, this speed-to-fly is calculated for maintaining altitude.  In final glide mode, this speed-to-fly is calculated for descent.  In climb mode, this switches to the speed for minimum sink at the current load factor (if an accelerometer is connected).  When Block mode speed to fly is selected, this infobox displays the MacCready speed."));
    break;
  case 44:
    dlgHelpShowModal(caption, TEXT("[Netto Vario]\r\nInstantaneous vertical speed of air-mass, equal to vario value less the glider's estimated sink rate.  Best used if airspeed, accelerometers and vario are connected, otherwise calculations are based on GPS measurements and wind estimates."));
    break;
  case 45:
    dlgHelpShowModal(caption, TEXT("[Task Arrival Time]\r\nEstimated arrival local time at task completion, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 46:
    dlgHelpShowModal(caption, TEXT("[Next Arrival Time]\r\nEstimated arrival local time at next waypoint, assuming performance of ideal MacCready cruise/climb cycle."));
    break;
  case 47:
    dlgHelpShowModal(caption, TEXT("[Bearing Difference]\r\nThe difference between the glider's track bearing, to the bearing of the next waypoint, or for AAT tasks, to the bearing to the target within the AAT sector.  GPS navigation is based on the track bearing across the ground, and this track bearing may differ from the glider's heading when there is wind present.  Chevrons point to the direction the glider needs to alter course to correct the bearing difference, that is, so that the glider's course made good is pointing directly at the next waypoint.  This bearing takes into account the curvature of the Earth."));
    break;
  case 48:
    dlgHelpShowModal(caption, TEXT("[Outside Air Temperature]\r\nOutside air temperature measured by a probe if supported by a connected intelligent variometer."));
    break;
  case 49:
    dlgHelpShowModal(caption, TEXT("[Relative Humidity]\r\nRelative humidity of the air in percent as measured by a probe if supported by a connected intelligent variometer."));
    break;
  case 50:
    dlgHelpShowModal(caption, TEXT("[Forecast Temperature]\r\nForecast temperature of the ground at the home airfield, used in estimating convection height and cloud base in conjunction with outside air temperature and relative humidity probe.  (Touchscreen/PC only) Pressing the up/down cursor keys adjusts this forecast temperature."));
    break;
  case 51:
    dlgHelpShowModal(caption, TEXT("[AA Distance Tgt]\r\nAssigned Area Task distance around target points for remainder of task."));
    break;
  case 52:
    dlgHelpShowModal(caption, TEXT("[AA Speed Tgt]\r\nAssigned Area Task average speed achievable around target points remaining in minimum AAT time."));
    break;
  case 53:
    dlgHelpShowModal(caption, TEXT("[L/D vario]\r\nInstantaneous glide ratio, given by the indicated airspeed divided by the total energy vertical speed, when connected to an intelligent variometer.  Negative values indicate climbing cruise. If the total energy vario speed is close to zero, the displayed value is '---'."));
    break;
  case 54:
    dlgHelpShowModal(caption, TEXT("[Airspeed TAS]\r\nTrue Airspeed reported by a supported external intelligent vario."));
    break;
  case 55:
    dlgHelpShowModal(caption, TEXT("[Own Team Code]\r\nThe current Team code for this aircraft.  Use this to report to other team members."));
    break;
  case 56:
    dlgHelpShowModal(caption, TEXT("[Team Bearing]\r\nThe bearing to the team aircraft location at the last team code report."));
    break;
  case 57:
    dlgHelpShowModal(caption, TEXT("[Team Bearing Diff]\r\nThe relative bearing to the team aircraft location at the last reported team code."));
    break;
  case 58:
    dlgHelpShowModal(caption, TEXT("[Team range]\r\nThe range to the team aircraft location at the last reported team code."));
    break;
  case 59:
    dlgHelpShowModal(caption, TEXT("[Speed Task Instantaneous]\r\nInstantaneous cross country speed while on current task, compensated for altitude."));
    break;
  case 60:
    dlgHelpShowModal(caption, TEXT("[Distance Home]\r\nDistance to home waypoint (if defined)."));
    break;
  case 61:
    dlgHelpShowModal(caption, TEXT("[Speed Task Achieved]\r\nAchieved cross country speed while on current task, compensated for altitude."));
    break;
  case 62:
    dlgHelpShowModal(caption, TEXT("[AA Delta Time]\r\nDifference between estimated task time and AAT minimum time.  Colored red if negative (expected arrival too early), or blue if in sector and can turn now with estimated arrival time greater than AAT time plus 5 minutes."));
    break;
  case 63:
    dlgHelpShowModal(caption, TEXT("[Thermal All]\r\nTime-average climb rate in all thermals."));
    break;
  case 65:
    dlgHelpShowModal(caption, TEXT("[Battery]\r\nSupply battery voltage for Altair systems, for PDA systems, this gives the percent battery capacity available."));
    break;
  case 66:
    dlgHelpShowModal(caption, TEXT("[Final GR]\r\nGeometric gradient to the arrival height above the final waypoint.  This is not adjusted for total energy."));
    break;
  case 68:
    dlgHelpShowModal(caption, TEXT("[Alternate GR]\r\nGeometric gradient to the arrival height above the selected alternate waypoint.  This is not adjusted for total energy.\r\nFor values over 200, nothing is shown, for values between 100 and 200 an integer number is shown, and for values between 1 and 99 a decimal value is shown.\r\nThe alternate value is RED if:\r\n- above the best LD or unreachable (terrain, wind, arrival altitude is less than 100m over the safety altitude)\r\nThe bottom line changes every 3 seconds showing distance or arrival altitude over safety.\r\n All values are calculated in real-time\r\nPress the ENTER key with the infobox selected to bring up the relative waypoint info page: here you can perform actions on the alternate such as goto, insert in task etc."));
    break;
  case 69:
    dlgHelpShowModal(caption, TEXT("[Best Alternate]\r\nThis automatically searches for the best landing option available. The top line is the name of the landpoint, the middle value is the LD required, and the bottom line displays the distance and arrival altitude over safety (cycled every few seconds). Please refer to the manual to see how the Best Alternate works and how you can customise its behaviour."));
    break;
  case 70:
    dlgHelpShowModal(caption, TEXT("[QFE]\r\nAutomatic QFE altitude based on GPS altitude. This altitude value is constantly reset to 0 on ground before taking off. After takeoff, it no longer is reset automatically.  During flight you can adjust the QFE value with the up and down keys. The bottom line shows QNH altitude. \r\nChanging QFE does not affect QNH altitude or any other glide calculation."));
    break;
  case 67:
  default:
    dlgHelpShowModal(caption, TEXT("No help available on this item!"));
  };
}
