/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Screen/Color.hpp"
#include "Screen/Brush.hpp"
#include "Screen/ContainerWindow.hpp"
#include "Util/StaticString.hpp"

#include <tchar.h>

/**
 * The WindowControl class is the base class for every other control
 * including the forms/windows itself, using the ContainerControl.
 */
class WindowControl : public ContainerWindow {
public:
  typedef void (*OnHelpCallback_t)(WindowControl *Sender);

protected:
  /** Caption/Text of the Control */
  StaticString<254> mCaption;

private:
  /** Background color */
  Color mColorBack;
  /** Foreground color */
  Color mColorFore;

  /** Font of the Control */
  const Font *mhFont;

  /** Helptext of the Control */
  TCHAR *mHelpText;

  /**
   * The callback-function that should be called when the help button is
   * pressed while the control has focus
   * @see SetOnHelpCallback()
   */
  OnHelpCallback_t mOnHelpCallback;

public:
  WindowControl();

  /** Destructor */
  virtual ~WindowControl(void);

  /**
   * The on_setfocus event is called when the control gets focused
   * (derived from Window)
   */
  virtual bool on_setfocus();
  /**
   * The on_killfocus event is called when the control loses the focus
   * (derived from Window)
   */
  virtual bool on_killfocus();

  /**
   * The on_key_down event is called when a key is pressed while the
   * control is focused
   * (derived from Window)
   */
  virtual bool on_key_down(unsigned key_code);

  /**
   * The on_key_up event is called when a key is released while the
   * control is focused
   * (derived from Window)
   */
  virtual bool on_key_up(unsigned key_code);

  /**
   * Opens up a help dialog if a help text exists, or otherwise calls the
   * function defined by mOnHelpCallback
   * @return
   */
  int OnHelp();

  /**
   * Sets the function that should be called when the help button is pressed
   * @param Function Pointer to the function to be called
   */
  void SetOnHelpCallback(OnHelpCallback_t Function) {
    mOnHelpCallback = Function;
  }

  /**
   * Returns the Control's font
   * @return The Control's font
   */
  const Font *GetFont() const {
    return mhFont;
  }

  /**
   * Sets the Control's font
   * @param font The new font
   * @return The old font
   */
  virtual void SetFont(const Font &font);

  /**
   * Sets the Control's font
   * @param font The new font
   * @return The old font
   */
  void SetFont(const Font *font) {
    SetFont(*font);
  }

  /**
   * Sets the Control's foreground color
   * @param Value The new foreground color
   * @return The old foreground color
   */
  void SetForeColor(Color Value);

  /**
   * Return the Control's foreground color
   * @return The Control's foreground color
   */
  Color GetForeColor() const {
    return mColorFore;
  }

  /**
   * Sets the Control's background color
   * @param Value The new background color
   * @return The old background color
   */
  void SetBackColor(Color Value);

  /**
   * Return the Control's background color
   * @return The Control's background color
   */
  Color GetBackColor() const {
    return mColorBack;
  }

  /**
   * Returns the Caption/Text of the Control
   * @return The Caption/Text of the Control
   */
  const TCHAR *GetCaption() const {
    return mCaption.c_str();
  }

  /**
   * Sets the Caption/Text of the Control
   * @param Value The new Caption/Text of the Control
   */
  void SetCaption(const TCHAR *Value);

  /**
   * Sets the Helptext of the Control
   * @param Value The new Helptext of the Control
   */
  void SetHelpText(const TCHAR *Value);

  /** Paints the "Selector" */
  static void PaintSelector(Canvas &canvas, const PixelRect rc);
};

#endif
