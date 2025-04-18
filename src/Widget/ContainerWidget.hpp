// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"
#include "ui/window/ContainerWindow.hpp"

/**
 * Utility base class that creates a ContainerWindow, allowing the
 * derived class to add child windows to it.
 */
class ContainerWidget : public WindowWidget {
protected:
  ContainerWindow &GetContainer() noexcept {
    return (ContainerWindow &)GetWindow();
  }

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
