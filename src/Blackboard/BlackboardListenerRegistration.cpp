// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BlackboardListenerRegistration.hpp"

#include "BlackboardListener.hpp"
#include "LiveBlackboard.hpp"

BlackboardListenerRegistration::~BlackboardListenerRegistration() noexcept
{
  Reset();
}

void
BlackboardListenerRegistration::Register(LiveBlackboard &_blackboard,
                                         BlackboardListener &_listener) noexcept
{
  if (blackboard == &_blackboard && listener == &_listener)
    return;

  Reset();

  blackboard = &_blackboard;
  listener = &_listener;
  blackboard->AddListener(*listener);
}

void
BlackboardListenerRegistration::Reset() noexcept
{
  if (blackboard == nullptr || listener == nullptr)
    return;

  blackboard->RemoveListener(*listener);
  blackboard = nullptr;
  listener = nullptr;
}
