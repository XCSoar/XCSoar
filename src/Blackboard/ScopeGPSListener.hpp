// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BlackboardListener.hpp"
#include "LiveBlackboard.hpp"

#include <functional>
#include <cassert>

/**
 * A dummy class that implements all abstract methods as no-ops.
 * Inherit this class and only implement the methods you're interested
 * in.
 */
class ScopeGPSListener : public NullBlackboardListener {
public:
  typedef std::function<void(const MoreData &basic)> Function;

private:
  LiveBlackboard &blackboard;
  Function function;

public:
  ScopeGPSListener(LiveBlackboard &_blackboard, Function _function)
    :blackboard(_blackboard), function(_function) {
    assert(function);

    blackboard.AddListener(*this);
  }

  ~ScopeGPSListener() {
    blackboard.RemoveListener(*this);
  }

  virtual void OnGPSUpdate(const MoreData &basic) override;
};
