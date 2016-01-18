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

#ifndef XCSOAR_FORM_CHECK_BOX_HPP
#define XCSOAR_FORM_CHECK_BOX_HPP

#include "Screen/PaintWindow.hpp"
#include "Util/tstring.hpp"

struct DialogLook;
class ContainerWindow;
class ActionListener;

/**
 * This class is used for creating buttons.
 */
class CheckBoxControl : public PaintWindow {
  bool checked, dragging, pressed;

  const DialogLook *look;
  tstring caption;

  ActionListener *listener;
  int id;

public:
  void Create(ContainerWindow &parent, const DialogLook &look,
              tstring::const_pointer caption,
              const PixelRect &rc,
              const WindowStyle style,
              ActionListener &listener, int id);

  /**
   * Set the object that will receive click events.
   */
  void SetListener(ActionListener &_listener) {
    assert(listener == nullptr);

    listener = &_listener;
  }

  bool GetState() const {
    return checked;
  }

  void SetState(bool value);

protected:
  void SetPressed(bool value);

  virtual bool OnClicked();

  /* virtual methods from class Window */
  bool OnKeyCheck(unsigned key_code) const override;
  bool OnKeyDown(unsigned key_code) override;
  bool OnMouseMove(PixelPoint p, unsigned keys) override;
  bool OnMouseDown(PixelPoint p) override;
  bool OnMouseUp(PixelPoint p) override;
  void OnSetFocus() override;
  void OnKillFocus() override;
  void OnCancelMode() override;
  void OnPaint(Canvas &canvas) override;
};

#endif
