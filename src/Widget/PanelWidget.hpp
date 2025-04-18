// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "WindowWidget.hpp"

/**
 * Class to be inherited.
 * Window widget with a panel the size of the rect passed to Prepare().
 * Panel is automatically created by Prepare().
 * Panel is accessed via WindowWidget's GetWindow().
 */
class PanelWidget : public WindowWidget {
public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};
