// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"
#include "Form/DataField/Listener.hpp"
#include "Blackboard/RateLimitedBlackboardListener.hpp"

class TaskStatusPanel
 : public StatusPanel, DataFieldListener,
   private NullBlackboardListener {
  RateLimitedBlackboardListener rate_limiter;


public:
  explicit TaskStatusPanel(const DialogLook &look) noexcept
    :StatusPanel(look), rate_limiter(*this, std::chrono::seconds(1),
                                     std::chrono::milliseconds(500)) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
  /* virtual methods from class BlackboardListener */
  void OnCalculatedUpdate(const MoreData &basic, const DerivedInfo &calculated) override;
};
