// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "BlackboardListener.hpp"

/**
 * A #BlackboardListener that forwards all calls to another listener.
 * Use this as a base class for specialised implementations.
 */
class ProxyBlackboardListener : public BlackboardListener {
protected:
  BlackboardListener &next;

public:
  ProxyBlackboardListener(BlackboardListener &_next):next(_next) {}

private:
  virtual void OnGPSUpdate(const MoreData &basic);

  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated);

  virtual void OnComputerSettingsUpdate(const ComputerSettings &settings);

  virtual void OnUISettingsUpdate(const UISettings &settings);
};
