// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "util/StaticString.hxx"

#include <tchar.h>

struct DialogLook;

/**
 * The WindowControl class is the base class for every other control
 * including the forms/windows itself, using the ContainerControl.
 */
class WindowControl : public PaintWindow {
protected:
  /** Caption/Text of the Control */
  StaticString<254> caption;

private:
  /** Helptext of the Control */
  const char *help_text = nullptr;

public:
  WindowControl() noexcept;

  /**
   * Does this control have a help text?
   */
  bool HasHelp() const noexcept {
    return help_text != nullptr;
  }

  /**
   * Opens up a help dialog if a help text exists
   */
  bool OnHelp() noexcept;

  bool HasCaption() const noexcept {
    return !caption.empty();
  }

  /**
   * Returns the Caption/Text of the Control
   * @return The Caption/Text of the Control
   */
  const char *GetCaption() const noexcept {
    return caption.c_str();
  }

  /**
   * Sets the Caption/Text of the Control
   * @param Value The new Caption/Text of the Control
   */
  void SetCaption(const char *Value) noexcept;

  /**
   * Sets the Helptext of the Control
   * @param Value The new Helptext of the Control
   */
  void SetHelpText(const char *_help_text) noexcept {
    help_text = _help_text;
  }

  const char *GetHelpText() const noexcept {
    return help_text;
  }
};
