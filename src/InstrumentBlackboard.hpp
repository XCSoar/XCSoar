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

#ifndef INSTRUMENT_BLACKBOARD_H
#define INSTRUMENT_BLACKBOARD_H

#include "InterfaceBlackboard.hpp"

class InstrumentBlackboard {
protected:
  static const NMEA_INFO& Basic() { return blackboard.Basic(); }
  static const DERIVED_INFO& Calculated() { return blackboard.Calculated(); }
  const SETTINGS_COMPUTER& SettingsComputer()
  { return blackboard.SettingsComputer(); }
public:
  static void ReadBlackboardBasic(const NMEA_INFO& nmea_info) {
    blackboard.ReadBlackboardBasic(nmea_info);
  }
  static void ReadBlackboardCalculated(const DERIVED_INFO& derived_info) {
    blackboard.ReadBlackboardCalculated(derived_info);
  }
  static void ReadSettingsComputer(const SETTINGS_COMPUTER &settings) {
    blackboard.ReadSettingsComputer(settings);
  }
private:
  static InterfaceBlackboard blackboard;
};

#endif
