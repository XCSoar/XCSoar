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

#ifndef XCSOAR_WIDGET_DIALOG_HPP
#define XCSOAR_WIDGET_DIALOG_HPP

#include "Screen/Point.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Form/ManagedWidget.hpp"

#include <tchar.h>

class Widget;

class WidgetDialog : public WndForm {
  ButtonPanel buttons;

  ManagedWidget widget;

  bool auto_size;

  bool changed;

public:
  WidgetDialog(const TCHAR *caption, const PixelRect &rc, Widget *widget);

  /**
   * Create a dialog with an automatic size (by
   * Widget::GetMinimumSize() and Widget::GetMaximumSize()).
   */
  WidgetDialog(const TCHAR *caption, Widget *widget);

  bool GetChanged() const {
    return changed;
  }

  Widget &GetWidget() {
    assert(widget.IsDefined());
    return *widget.Get();
  }

  Widget *StealWidget() {
    assert(widget.IsDefined());
    widget.Unprepare();
    return widget.Steal();
  }

  WndButton *AddButton(const TCHAR *caption,
                       WndButton::ClickNotifyCallback callback) {
    return buttons.Add(caption, callback);
  }

  WndButton *AddButton(const TCHAR *caption,
                       ActionListener *listener, int id) {
    return buttons.Add(caption, listener, id);
  }

  WndButton *AddButton(const TCHAR *caption, int modal_result) {
    return AddButton(caption, this, modal_result);
  }

  int ShowModal();

  virtual void OnAction(int id);

private:
  void AutoSize();

protected:
  virtual void OnDestroy();
  virtual void OnResize(UPixelScalar width, UPixelScalar height);
};

/**
 * Show a #Widget in a dialog, with OK and Cancel buttons.
 *
 * @param widget the #Widget to be displayed; it is not "prepared" and
 * will be "unprepared" (but "initialised") before returning; the
 * caller is responsible for destructing it
 * @return true if changed data was saved
 */
bool
DefaultWidgetDialog(const TCHAR *caption, const PixelRect &rc, Widget &widget);

bool
DefaultWidgetDialog(const TCHAR *caption, Widget &widget);

#endif
