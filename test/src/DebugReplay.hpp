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

#ifndef XCSOAR_DEBUG_REPLAY_HPP
#define XCSOAR_DEBUG_REPLAY_HPP

#include "SettingsComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"
#include "Computer/BasicComputer.hpp"
#include "Computer/FlyingComputer.hpp"
#include "Engine/GlideSolvers/GlidePolar.hpp"

class NLineReader;
class Device;
struct DeviceRegister;
class Args;

class DebugReplay {
protected:
  NLineReader *reader;

  GlidePolar glide_polar;

  BasicComputer computer;
  FlyingComputer flying_computer;

  SETTINGS_COMPUTER settings_computer;
  MoreData basic, last_basic;
  DerivedInfo calculated, last_calculated;

public:
  DebugReplay(NLineReader *reader);
  virtual ~DebugReplay();

  virtual bool Next() = 0;

  const SETTINGS_COMPUTER &SettingsComputer() const {
    return settings_computer;
  }

  const MoreData &Basic() const {
    return basic;
  }

  const MoreData &LastBasic() const {
    return last_basic;
  }

  const DerivedInfo &Calculated() const {
    return calculated;
  }

  const DerivedInfo &LastCalculated() const {
    return last_calculated;
  }

  DerivedInfo &SetCalculated() {
    return calculated;
  }

protected:
  void Compute();
};

DebugReplay *
CreateDebugReplay(Args &args);

#endif
