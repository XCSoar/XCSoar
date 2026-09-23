// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/ContainerWindow.hpp"

struct DialogLook;

/**
 * The PanelControl class implements the simplest form of a ContainerControl.
 *
 * The window stays transparent. The parent has already painted the
 * background, and a second fill would restart the dialog gradient.
 */
class PanelControl : public ContainerWindow
{
public:
  PanelControl() = default;

  /**
   * Constructor of the PanelControl class
   * @param owner Parent ContainerControl
   */
  PanelControl(ContainerWindow &parent, const DialogLook &look,
               const PixelRect &rc,
               const WindowStyle style=WindowStyle()) {
    Create(parent, look, rc, style);
  }

  void Create(ContainerWindow &parent, const DialogLook &look,
              const PixelRect &rc,
              const WindowStyle style=WindowStyle());
};
