/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_FORM_CONTROL_HPP
#define XCSOAR_FORM_CONTROL_HPP

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
  const TCHAR *help_text = nullptr;

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
  const TCHAR *GetCaption() const noexcept {
    return caption.c_str();
  }

  /**
   * Sets the Caption/Text of the Control
   * @param Value The new Caption/Text of the Control
   */
  void SetCaption(const TCHAR *Value) noexcept;

  /**
   * Sets the Helptext of the Control
   * @param Value The new Helptext of the Control
   */
  void SetHelpText(const TCHAR *_help_text) noexcept {
    help_text = _help_text;
  }

  const TCHAR *GetHelpText() const noexcept {
    return help_text;
  }
};

#endif
