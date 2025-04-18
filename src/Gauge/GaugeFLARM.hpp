// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/OverlappedWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

class LiveBlackboard;
struct FlarmTrafficLook;
struct NMEAInfo;

/**
 * Widget to display a FLARM gauge
 */
class GaugeFLARM final : public OverlappedWidget, NullBlackboardListener {
  LiveBlackboard &blackboard;
  const FlarmTrafficLook &look;

public:
  GaugeFLARM(LiveBlackboard &_blackboard,
             const FlarmTrafficLook &_look) noexcept
    :blackboard(_blackboard), look(_look) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  void Update(const NMEAInfo &basic) noexcept;

  void OnGPSUpdate(const MoreData &basic) override;
};
