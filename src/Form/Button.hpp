/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Screen/PaintWindow.hpp"

#include <tchar.h>

struct ButtonLook;
class ContainerWindow;
class ActionListener;
class ButtonRenderer;

/**
 * This class is used for creating buttons.
 */
class Button : public PaintWindow {
  bool dragging, down;

  ButtonRenderer *renderer;

  ActionListener *listener;
  int id;

  /**
   * This flag specifies whether the button is "selected".  The
   * "selected" button in a #ButtonPanel is the button that will be
   * triggered by the #KEY_RETURN.  On some devices without touch
   * screen, cursor keys left/right can be used to navigate the
   * #ButtonPanel.
   */
  bool selected;

public:
  Button(ContainerWindow &parent, const PixelRect &rc,
         WindowStyle style, ButtonRenderer *_renderer,
         ActionListener &_listener, int _id) {
    Create(parent, rc, style, _renderer, _listener, _id);
  }

  Button(ContainerWindow &parent, const ButtonLook &look,
         const TCHAR *caption, const PixelRect &rc,
         WindowStyle style,
         ActionListener &_listener, int _id) {
    Create(parent, look, caption, rc, style, _listener, _id);
  }

  Button():listener(nullptr) {}

  virtual ~Button();

  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style, ButtonRenderer *_renderer);

  void Create(ContainerWindow &parent, const ButtonLook &look,
              const TCHAR *caption, const PixelRect &rc,
              WindowStyle style);

  void Create(ContainerWindow &parent, const PixelRect &rc,
              WindowStyle style, ButtonRenderer *_renderer,
              ActionListener &listener, int id);

  void Create(ContainerWindow &parent, const ButtonLook &look,
              const TCHAR *caption, const PixelRect &rc,
              WindowStyle style,
              ActionListener &listener, int id);

  /**
   * Set the object that will receive click events.
   */
  void SetListener(ActionListener &_listener, int _id) {
    id = _id;
    listener = &_listener;
  }

  ButtonRenderer &GetRenderer() {
    return *renderer;
  }

  /**
   * Set a new caption.  This method is a wrapper for
   * #TextButtonRenderer and may only be used if created with a
   * #TextButtonRenderer instance.
   */
  void SetCaption(const TCHAR *caption);

  void SetSelected(bool _selected);

  gcc_pure
  unsigned GetMinimumWidth() const;

  /**
   * Simulate a click on this button.
   */
  void Click();

protected:
  /**
   * Called when the button is clicked (either by mouse or by
   * keyboard).  The default implementation invokes the OnClick
   * callback.
   */
  virtual bool OnClicked();

/* virtual methods from class Window */
  void OnDestroy() override;

  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  void OnSetFocus() override;
  void OnKillFocus() override;
  void OnCancelMode() override;

  void OnPaint(Canvas &canvas) override;

private:
  void SetDown(bool _down);
};

#endif
