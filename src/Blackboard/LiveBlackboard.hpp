// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
