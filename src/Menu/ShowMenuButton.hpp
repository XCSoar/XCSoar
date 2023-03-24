// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Button.hpp"

class ShowMenuButton : public Button {
public:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};
