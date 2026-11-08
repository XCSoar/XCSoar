// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Blackboard/RateLimitedBlackboardListener.hpp"

#include <chrono>

class HorizonWidget final : public WindowWidget,
                            private NullBlackboardListener {
  /* Paint each new attitude as it arrives, and no faster than 30 Hz. */
  RateLimitedBlackboardListener rate_limiter;

  void Update(const MoreData &basic) noexcept;

public:
  HorizonWidget() noexcept
    :rate_limiter(*this, std::chrono::milliseconds(33), {}) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;

private:
  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) noexcept override;
};
