// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"

#include <functional>
#include <utility>

class Button;

class TimesStatusPanel : public StatusPanel {
  Button *timer_button = nullptr;
  std::function<void()> close_dialog;

public:
  TimesStatusPanel(const DialogLook &look,
                   std::function<void()> _close_dialog) noexcept
    :StatusPanel(look), close_dialog(std::move(_close_dialog)) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
