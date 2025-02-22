// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Button.hpp"

/* "M" menu button */
class ShowMenuButton : public Button {
public:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};

/* zoom out button */
class ShowZoomOutButton : public Button {
public:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};

/* zoom in button */
class ShowZoomInButton : public Button {
public:
  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style=WindowStyle()) noexcept;

protected:
  /* virtual methods from class ButtonWindow */
  bool OnClicked() noexcept override;
};
