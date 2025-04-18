// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/OverlappedWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"

struct AttitudeState;
class LiveBlackboard;
struct ThermalAssistantLook;

/**
 * Widget to display a FLARM gauge
 */
class GaugeThermalAssistant final
  : public OverlappedWidget, private NullBlackboardListener {
  LiveBlackboard &blackboard;
  const ThermalAssistantLook &look;

public:
  GaugeThermalAssistant(LiveBlackboard &_blackboard,
                        const ThermalAssistantLook &_look) noexcept
    :blackboard(_blackboard), look(_look) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool SetFocus() noexcept override;

private:
  void Update(const AttitudeState &attitude,
              const DerivedInfo &calculated) noexcept;

  virtual void OnCalculatedUpdate(const MoreData &basic,
                                  const DerivedInfo &calculated) override;
};
