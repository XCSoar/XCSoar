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

#ifndef XCSOAR_WIDGET_DIALOG_HPP
#define XCSOAR_WIDGET_DIALOG_HPP

#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ManagedWidget.hpp"

#include <tchar.h>

class Widget;

class WidgetDialog : public WndForm {
  ButtonPanel buttons;

  ManagedWidget widget;

  bool full;

  bool auto_size;

  bool changed;

public:
  explicit WidgetDialog(const DialogLook &look);

  virtual ~WidgetDialog();

  const ButtonLook &GetButtonLook() const {
    return buttons.GetLook();
  }

  void Create(SingleWindow &parent, const TCHAR *caption,
              const PixelRect &rc, Widget *widget);

  /**
   * Create a full-screen dialog.
   */
  void CreateFull(SingleWindow &parent, const TCHAR *caption, Widget *widget);

  /**
   * Create a dialog with an automatic size (by
   * Widget::GetMinimumSize() and Widget::GetMaximumSize()).
   */
  void CreateAuto(SingleWindow &parent, const TCHAR *caption, Widget *widget);

  /**
   * Create a dialog, but do not associate it with a #Widget yet.
   * Call FinishPreliminary() to resume building the dialog.
   */
  void CreatePreliminary(SingleWindow &parent, const TCHAR *caption);

  void FinishPreliminary(Widget *widget);

  bool GetChanged() const {
    return changed;
  }

  /**
   * Ensure that the widget is prepared.
   */
  void PrepareWidget() {
    widget.Prepare();
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

  Button *AddButton(ButtonRenderer *renderer,
                    ActionListener &listener, int id) {
    return buttons.Add(renderer, listener, id);
  }

  Button *AddButton(const TCHAR *caption,
                    ActionListener &listener, int id) {
    return buttons.Add(caption, listener, id);
  }

  Button *AddButton(const TCHAR *caption, int modal_result) {
    return AddButton(caption, *this, modal_result);
  }

  Button *AddSymbolButton(const TCHAR *caption,
                          ActionListener &listener, int id) {
    return buttons.AddSymbol(caption, listener, id);
  }

  void AddButtonKey(unsigned key_code) {
    return buttons.AddKey(key_code);
  }

  /**
   * @see ButtonPanel::EnableCursorSelection()
   */
  void EnableCursorSelection(unsigned _index=0) {
    buttons.EnableCursorSelection(_index);
  }

  int ShowModal();

  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;

private:
  void AutoSize();

protected:
  /* virtual methods from class Window */
  virtual void OnDestroy() override;
  virtual void OnResize(PixelSize new_size) override;

  /* virtual methods from class WndForm */
  virtual void ReinitialiseLayout(const PixelRect &parent_rc) override;
  virtual void SetDefaultFocus() override;
  virtual bool OnAnyKeyDown(unsigned key_code) override;
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
DefaultWidgetDialog(SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, const PixelRect &rc, Widget &widget);

bool
DefaultWidgetDialog(SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, Widget &widget);

#endif
