// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

struct VarioLook;
struct VarioBarLook;
class LiveBlackboard;

/**
 * A variant of GaugeVario which auto-updates its data from the device
 * blackboard.
 */
class GlueGaugeVario final
  : public WindowWidget, private NullBlackboardListener {
  LiveBlackboard &blackboard;
  VarioLook &look;
  const VarioBarLook &vario_bar_look;

public:
  GlueGaugeVario(LiveBlackboard &_blackboard, VarioLook &_look,
                 const VarioBarLook &_vario_bar_look) noexcept
    :blackboard(_blackboard), look(_look), vario_bar_look(_vario_bar_look) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  virtual void OnGPSUpdate(const MoreData &basic) override;
};
