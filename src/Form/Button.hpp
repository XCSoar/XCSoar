/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_FORM_BUTTON_HPP
#define XCSOAR_FORM_BUTTON_HPP

#include "Screen/ButtonWindow.hpp"
#include "Renderer/ButtonRenderer.hpp"

#include <assert.h>

struct ButtonLook;
class ContainerWindow;
class ActionListener;

/**
 * This class is used for creating buttons.
 * It is based on the WindowControl class.
 */
class WndButton : public ButtonWindow {
public:
  typedef void (*ClickNotifyCallback)();

protected:
  ButtonRenderer renderer;

private:
#ifdef USE_GDI
  int id;
#endif

  ActionListener *listener;

  /**
   * The callback-function that should be called when the button is pressed
   * @see SetOnClickNotify()
   */
  ClickNotifyCallback click_callback;

public:
  /**
   * Constructor of the WndButton class
   * @param Parent Parent window/ContainerControl
   * @param Caption Text on the button
   * @param Function The function that should be called
   * when the button is clicked
   */
  WndButton(ContainerWindow &parent, const ButtonLook &look,
            tstring::const_pointer caption, const PixelRect &rc,
            ButtonWindowStyle style,
            ClickNotifyCallback click_callback = NULL);

  WndButton(ContainerWindow &parent, const ButtonLook &look,
            tstring::const_pointer caption, const PixelRect &rc,
            ButtonWindowStyle style,
            ActionListener &listener, int id);

  WndButton(const ButtonLook &_look);

  void Create(ContainerWindow &parent,
              tstring::const_pointer caption, const PixelRect &rc,
              ButtonWindowStyle style,
              ClickNotifyCallback click_callback=nullptr);

  void Create(ContainerWindow &parent,
              tstring::const_pointer caption, const PixelRect &rc,
              ButtonWindowStyle style,
              ActionListener &listener, int id);

  /**
   * Set the object that will receive click events.
   */
  void SetListener(ActionListener *_listener, int _id) {
    assert(click_callback == NULL);

#ifdef USE_GDI
    id = _id;
#else
    SetID(_id);
#endif
    listener = _listener;
  }

  /**
   * Sets the function that should be called when the button is pressed
   * @param Function Pointer to the function to be called
   */
  void
  SetOnClickNotify(ClickNotifyCallback _click_callback)
  {
    assert(listener == NULL);

    click_callback = _click_callback;
  }

  /**
   * Sets the Caption/Text of the Control and resets the cached text height
   * (derived from WindowControl)
   * @param Value The new Caption/Text of the Control
   */
  void SetCaption(tstring::const_pointer caption) {
    SetText(caption);
  }

  /**
   * Called when the button is clicked (either by mouse or by
   * keyboard).  The default implementation invokes the OnClick
   * callback.
   */
  virtual bool OnClicked() override;

protected:
#ifdef USE_GDI
  virtual void OnSetFocus() override;
  virtual void OnKillFocus() override;
#endif

  virtual void OnPaint(Canvas &canvas) override;
};

#endif
