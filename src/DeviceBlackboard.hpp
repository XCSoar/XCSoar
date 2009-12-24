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

#ifndef DEVICE_BLACKBOARD_H
#define DEVICE_BLACKBOARD_H

#include "Blackboard.hpp"
#include "SettingsComputerBlackboard.hpp"
#include "SettingsMapBlackboard.hpp"
#include "MapProjectionBlackboard.hpp"

class GlidePolar;

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
  void SetStartupLocation(const GEOPOINT &loc, const double alt);
  void SetLocation(const GEOPOINT &loc, const double speed, const double bearing,
		   const double alt, const double baroalt, const double t);
  void ProcessSimulation();
  bool LowerConnection();
  void RaiseConnection();
  void StopReplay();
  void SetBaroAlt(fixed x) {
    SetBasic().BaroAltitude = x;
  }
  void SetNAVWarning(bool val);
  void SetTrackBearing(fixed val);
  void SetSpeed(fixed val);
  void SetAltitude(fixed alt);

  void tick(const GlidePolar& glide_polar);
  void tick_fast(const GlidePolar& glide_polar);

private:
// moved from GlideComputerAirData
  void FLARM_RefreshSlots();
  void FLARM_ScanTraffic();
  void SetSystemTime();
  void NavAltitude();
  void Heading();
  void Dynamics();
  void Wind();
  void EnergyHeight();
  void TurnRate();
  void Vario();
  void NettoVario(const GlidePolar& glide_polar);
  void AutoQNH(const GlidePolar& glide_polar);

  NMEA_INFO state_last;
  const NMEA_INFO& LastBasic() { return state_last; }
};

extern DeviceBlackboard device_blackboard;

#endif
