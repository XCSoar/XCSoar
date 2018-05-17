/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef GLIDECOMPUTER_BLACKBOARD_HPP
#define GLIDECOMPUTER_BLACKBOARD_HPP

#include "Blackboard/BaseBlackboard.hpp"
#include "Blackboard/ComputerSettingsBlackboard.hpp"

/**
 * Blackboard class used by glide computer (calculation) thread.
 * Can only write DERIVED_INFO
 */
class GlideComputerBlackboard:
  public BaseBlackboard,
  public ComputerSettingsBlackboard
{
  DerivedInfo Finish_Derived_Info;

public:
  void ReadBlackboard(const MoreData &nmea_info);
  void ReadComputerSettings(const ComputerSettings &settings);

protected:
  void ResetFlight(const bool full=true);
  void StartTask();
  void SaveFinish();
  void RestoreFinish();

  // only the glide computer can write to calculated
  DerivedInfo& SetCalculated() { return calculated_info; }
};

#endif
