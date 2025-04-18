// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

struct VarioLook;
class LiveBlackboard;

/**
 * A variant of GaugeVario which auto-updates its data from the device
 * blackboard.
 */
class GlueGaugeVario final
  : public WindowWidget, private NullBlackboardListener {
  LiveBlackboard &blackboard;
  const VarioLook &look;

public:
  GlueGaugeVario(LiveBlackboard &_blackboard, const VarioLook &_look) noexcept
    :blackboard(_blackboard), look(_look) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  virtual void OnGPSUpdate(const MoreData &basic) override;
};
