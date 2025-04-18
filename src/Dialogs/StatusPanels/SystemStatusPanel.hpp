// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"
#include "Blackboard/RateLimitedBlackboardListener.hpp"

class SystemStatusPanel final
  : public StatusPanel,
    private NullBlackboardListener {
  RateLimitedBlackboardListener rate_limiter;

public:
  explicit SystemStatusPanel(const DialogLook &look) noexcept
    :StatusPanel(look), rate_limiter(*this, std::chrono::seconds(2),
                                     std::chrono::milliseconds(500)) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

private:
  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override;
};
