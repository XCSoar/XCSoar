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

#ifndef INTERFACE_BLACKBOARD_H
#define INTERFACE_BLACKBOARD_H

#include "LiveBlackboard.hpp"
#include "Compiler.h"

class InterfaceBlackboard : public LiveBlackboard
{
public:
  void ReadBlackboardBasic(const MoreData &nmea_info);
  void ReadBlackboardCalculated(const DerivedInfo &derived_info);

  gcc_const
  SystemSettings &SetSystemSettings() {
    return system_settings;
  }

  gcc_const
  ComputerSettings& SetComputerSettings() {
    return computer_settings;
  }

  gcc_const
  UISettings &SetUISettings() {
    return ui_settings;
  }

  inline void ReadCommonStats(const CommonStats &common_stats) {
    calculated_info.common_stats = common_stats;
  }

  void ReadComputerSettings(const ComputerSettings &settings);
};

#endif
