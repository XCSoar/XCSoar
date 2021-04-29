/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_LIVE_BLACKBOARD_HPP
#define XCSOAR_LIVE_BLACKBOARD_HPP

#include "FullBlackboard.hpp"

#include <list>

class BlackboardListener;

/**
 * A blackboard that supports #BlackboardListener.
 */
class LiveBlackboard : public FullBlackboard {
  std::list<BlackboardListener *> listeners;

#ifndef NDEBUG
  bool calling_listeners = false;
#endif

public:
  void AddListener(BlackboardListener &listener) noexcept;
  void RemoveListener(BlackboardListener &listener) noexcept;

  void BroadcastGPSUpdate() noexcept;
  void BroadcastCalculatedUpdate() noexcept;
  void BroadcastComputerSettingsUpdate() noexcept;
  void BroadcastUISettingsUpdate() noexcept;
};

#endif
