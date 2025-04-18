// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/canvas/Features.hpp"

#ifdef HAVE_CLIPPING
#include "ui/window/SolidContainerWindow.hpp"
#else
#include "ui/window/ContainerWindow.hpp"
#endif

struct DialogLook;

/**
 * The PanelControl class implements the simplest form of a ContainerControl
 */
class PanelControl :
#ifdef HAVE_CLIPPING
  public SolidContainerWindow
#else
  /* don't need to erase the background when it has been done by the
     parent window already */
  public ContainerWindow
#endif
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
