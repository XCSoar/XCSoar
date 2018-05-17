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

#include "LiveBlackboard.hpp"
#include "BlackboardListener.hpp"

#include <assert.h>

void
LiveBlackboard::AddListener(BlackboardListener &listener)
{
  assert(!calling_listeners);
  /* must not be registered already */
  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) == listeners.end());

  listeners.push_back(&listener);
}

void
LiveBlackboard::RemoveListener(BlackboardListener &listener)
{
  assert(!calling_listeners);

  auto i = std::find(listeners.begin(), listeners.end(), &listener);
  assert(i != listeners.end());

  listeners.erase(i);
}

void
LiveBlackboard::BroadcastGPSUpdate()
{
#ifndef NDEBUG
  calling_listeners = true;
#endif

  for (BlackboardListener *listener : listeners)
    listener->OnGPSUpdate(Basic());

#ifndef NDEBUG
  calling_listeners = false;
#endif
}

void
LiveBlackboard::BroadcastCalculatedUpdate()
{
#ifndef NDEBUG
  calling_listeners = true;
#endif

  for (BlackboardListener *listener : listeners)
    listener->OnCalculatedUpdate(Basic(), Calculated());

#ifndef NDEBUG
  calling_listeners = false;
#endif
}

void
LiveBlackboard::BroadcastComputerSettingsUpdate()
{
#ifndef NDEBUG
  calling_listeners = true;
#endif

  for (BlackboardListener *listener : listeners)
    listener->OnComputerSettingsUpdate(GetComputerSettings());

#ifndef NDEBUG
  calling_listeners = false;
#endif
}

void
LiveBlackboard::BroadcastUISettingsUpdate()
{
#ifndef NDEBUG
  calling_listeners = true;
#endif

  for (BlackboardListener *listener : listeners)
    listener->OnUISettingsUpdate(GetUISettings());

#ifndef NDEBUG
  calling_listeners = false;
#endif
}
