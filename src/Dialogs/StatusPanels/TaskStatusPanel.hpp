// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"
#include "Form/DataField/Listener.hpp"
#include "ui/event/Timer.hpp"

class TaskStatusPanel : public StatusPanel, DataFieldListener {

  UI::Timer refresh_timer{[this]{ Refresh(); }};

public:
  explicit TaskStatusPanel(const DialogLook &look) noexcept
    :StatusPanel(look) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  void Hide() noexcept override;

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};
