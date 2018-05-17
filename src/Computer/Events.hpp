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

#ifndef XCSOAR_GLIDE_COMPUTER_EVENTS_HPP
#define XCSOAR_GLIDE_COMPUTER_EVENTS_HPP

#include "Blackboard/BlackboardListener.hpp"
#include "NMEA/Validity.hpp"

/**
 * This class listens for #LiveBlackboard changes and emits glide
 * computer events.
 *
 * @see InputEvents::processGlideComputer()
 */
class GlideComputerEvents final : public NullBlackboardListener {
  bool enable_team, last_teammate_in_sector;

  bool last_flying;
  bool last_circling;
  bool last_final_glide;

  unsigned last_traffic;
  Validity last_new_traffic;

public:
  GlideComputerEvents():enable_team(false) {}

  void Reset();

  /* methods from BlackboardListener */
  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);
  virtual void OnComputerSettingsUpdate(const ComputerSettings &settings);
};

#endif
