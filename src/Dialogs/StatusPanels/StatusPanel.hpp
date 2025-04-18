// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"

class StatusPanel : public RowFormWidget {
public:
  StatusPanel(const DialogLook &look):RowFormWidget(look) {}

  virtual void Refresh() noexcept = 0;

  /* virtual methods from class Widget */
  void Show(const PixelRect &rc) noexcept override;
};
