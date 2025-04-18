// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"
#include "Form/DataField/Listener.hpp"

class TaskStatusPanel : public StatusPanel, DataFieldListener {
public:
  explicit TaskStatusPanel(const DialogLook &look) noexcept
    :StatusPanel(look) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};
