/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#ifndef DEVICE_BLACKBOARD_H
#define DEVICE_BLACKBOARD_H

#include "Blackboard.hpp"
#include "SettingsComputerBlackboard.hpp"
#include "SettingsMapBlackboard.hpp"
#include "MapProjectionBlackboard.hpp"

// the deviceblackboard is used as the global ground truth-state
// since it is accessed quickly with only one mutex (flight)
class DeviceBlackboard: 
  public BaseBlackboard,
  public SettingsComputerBlackboard,
  public SettingsMapBlackboard,
  public MapProjectionBlackboard
{
public:
  void Initialise();
  void ReadBlackboard(const DERIVED_INFO &derived_info);
  void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);
  void ReadSettingsMap(const SETTINGS_MAP &settings);

  // only the device blackboard can write to gps
  friend class ComPort;
protected:
  NMEA_INFO& SetBasic() { return gps_info; }
public:
  void SetStartupLocation(double lon, double lat, double alt);
  // used by replay logger
  void SetLocation(double lon, double lat, double speed, double bearing,
		   double alt, double baroalt, double t);
  void ProcessSimulation();
  bool LowerConnection(); // decrement
  void RaiseConnection(); // set to 2
  void StopReplay();
  void FLARM_RefreshSlots();
  void FLARM_ScanTraffic();
  void SetSystemTime();
  void SetBaroAlt(double x) {
    SetBasic().BaroAltitude = x;
  }
  void SetNAVWarning(bool val);
  void SetTrackBearing(double val);
  void SetSpeed(double val);
  void SetAltitude(double alt);
};

extern DeviceBlackboard device_blackboard;

#endif
