// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "StatusPanel.hpp"

class TimesStatusPanel : public StatusPanel {
public:
  explicit TimesStatusPanel(const DialogLook &look) noexcept
    :StatusPanel(look) {}

  /* virtual methods from class StatusPanel */
  void Refresh() noexcept override;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
