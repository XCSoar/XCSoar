// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class BlackboardListener;
class LiveBlackboard;

/**
 * Registers a #BlackboardListener on construction or #Register(), and
 * unregisters in #Reset() / the destructor.
 */
class BlackboardListenerRegistration {
  LiveBlackboard *blackboard = nullptr;
  BlackboardListener *listener = nullptr;

public:
  BlackboardListenerRegistration() noexcept = default;
  ~BlackboardListenerRegistration() noexcept;

  BlackboardListenerRegistration(const BlackboardListenerRegistration &) = delete;
  BlackboardListenerRegistration &
  operator=(const BlackboardListenerRegistration &) = delete;

  void Register(LiveBlackboard &_blackboard,
                BlackboardListener &_listener) noexcept;
  void Reset() noexcept;

  [[gnu::pure]]
  bool IsRegistered() const noexcept {
    return blackboard != nullptr;
  }
};
