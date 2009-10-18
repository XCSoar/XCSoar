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

#ifndef GLIDECOMPUTER_BLACKBOARD_HPP
#define GLIDECOMPUTER_BLACKBOARD_HPP

#include "Blackboard.hpp"
#include "SettingsComputerBlackboard.hpp"
#include "MapProjectionBlackboard.hpp"

class GlideComputerBlackboard:
  public BaseBlackboard,
  public SettingsComputerBlackboard,
  public MapProjectionBlackboard
{
public:
  void ReadBlackboard(const NMEA_INFO &nmea_info);
  void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);
  const NMEA_INFO& LastBasic() const { return last_gps_info; }
  const DERIVED_INFO& LastCalculated() const { return last_calculated_info; }
protected:
  bool time_advanced() {
    return (Basic().Time-LastBasic().Time>0);
  }
  /**
   * @see GlideComputerBlackboard::ReadBlackboard()
   */
  bool time_retreated() {
    return _time_retreated;
  }
  void ResetFlight(const bool full=true);
  void StartTask();
  void Initialise();
  void SaveFinish();
  void RestoreFinish();

  virtual double GetAverageThermal() const;
  virtual void OnClimbBase(double StartAlt) = 0;
  virtual void OnClimbCeiling() = 0;
  virtual void OnDepartedThermal() = 0;

  // only the glide computer can write to calculated
  DERIVED_INFO& SetCalculated() { return calculated_info; }
private:
  DERIVED_INFO Finish_Derived_Info;
  NMEA_INFO     last_gps_info;
  DERIVED_INFO last_calculated_info;
  bool _time_retreated;
};

#endif

