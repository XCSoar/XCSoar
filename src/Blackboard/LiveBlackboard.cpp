// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LiveBlackboard.hpp"
#include "BlackboardListener.hpp"

#include <cassert>

void
LiveBlackboard::AddListener(BlackboardListener &listener) noexcept
{
  assert(!calling_listeners);
  /* must not be registered already */
  assert(std::find(listeners.begin(), listeners.end(),
                   &listener) == listeners.end());

  listeners.push_back(&listener);
}

void
LiveBlackboard::RemoveListener(BlackboardListener &listener) noexcept
{
  assert(!calling_listeners);

  auto i = std::find(listeners.begin(), listeners.end(), &listener);
  assert(i != listeners.end());

  listeners.erase(i);
}

void
LiveBlackboard::BroadcastGPSUpdate() noexcept
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
LiveBlackboard::BroadcastCalculatedUpdate() noexcept
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
LiveBlackboard::BroadcastComputerSettingsUpdate() noexcept
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
LiveBlackboard::BroadcastUISettingsUpdate() noexcept
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
