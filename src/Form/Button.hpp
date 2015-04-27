/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

struct ButtonLook;
class ContainerWindow;
class ActionListener;
class ButtonRenderer;

/**
 * This class is used for creating buttons.
 */
class WndButton : public ButtonWindow {
  ButtonRenderer *renderer;

private:
#ifdef USE_GDI
  int id;
#endif

  ActionListener *listener;

public:
  WndButton(ContainerWindow &parent, const PixelRect &rc,
            ButtonWindowStyle style, ButtonRenderer *_renderer,
            ActionListener &_listener, int _id) {
    Create(parent, rc, style, _renderer, _listener, _id);
  }

  WndButton(ContainerWindow &parent, const ButtonLook &look,
            tstring::const_pointer caption, const PixelRect &rc,
            ButtonWindowStyle style,
            ActionListener &_listener, int _id) {
    Create(parent, look, caption, rc, style, _listener, _id);
  }

  WndButton():listener(nullptr) {}

  void Create(ContainerWindow &parent, const PixelRect &rc,
              ButtonWindowStyle style, ButtonRenderer *_renderer);

  void Create(ContainerWindow &parent, const ButtonLook &look,
              tstring::const_pointer caption, const PixelRect &rc,
              ButtonWindowStyle style);

  void Create(ContainerWindow &parent, const PixelRect &rc,
              ButtonWindowStyle style, ButtonRenderer *_renderer,
              ActionListener &listener, int id);

  void Create(ContainerWindow &parent, const ButtonLook &look,
              tstring::const_pointer caption, const PixelRect &rc,
              ButtonWindowStyle style,
              ActionListener &listener, int id);

  /**
   * Set the object that will receive click events.
   */
  void SetListener(ActionListener &_listener, int _id) {
#ifdef USE_GDI
    id = _id;
#else
    SetID(_id);
#endif
    listener = &_listener;
  }

  /**
   * Set a new caption.  This method is a wrapper for
   * #TextButtonRenderer and may only be used if created with a
   * #TextButtonRenderer instance.
   */
  void SetCaption(tstring::const_pointer caption);

  gcc_pure
  unsigned GetMinimumWidth() const;

  /**
   * Called when the button is clicked (either by mouse or by
   * keyboard).  The default implementation invokes the OnClick
   * callback.
   */
  virtual bool OnClicked() override;

protected:
  void OnDestroy() override;

#ifdef USE_GDI
  virtual void OnSetFocus() override;
  virtual void OnKillFocus() override;
#endif

  virtual void OnPaint(Canvas &canvas) override;
};

#endif
