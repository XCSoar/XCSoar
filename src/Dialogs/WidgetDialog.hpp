/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
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

#pragma once

#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/ManagedWidget.hpp"

#include <memory>
#include <type_traits>
#include <tchar.h>

class Widget;

class WidgetDialog : public WndForm {
  ButtonPanel buttons;

  ManagedWidget widget;

  bool full;

  bool auto_size;

  bool changed = false;

public:
  explicit WidgetDialog(const DialogLook &look);

  WidgetDialog(UI::SingleWindow &parent, const DialogLook &look,
               const PixelRect &rc, const TCHAR *caption,
               Widget *widget) noexcept;

  struct Auto {};

  /**
   * Create a dialog, but do not associate it with a #Widget yet.
   * Call FinishPreliminary() to resume building the dialog.
   */
  WidgetDialog(Auto, UI::SingleWindow &parent, const DialogLook &look,
               const TCHAR *caption) noexcept;

  /**
   * Create a dialog with an automatic size (by
   * Widget::GetMinimumSize() and Widget::GetMaximumSize()).
   */
  WidgetDialog(Auto, UI::SingleWindow &parent, const DialogLook &look,
               const TCHAR *caption, Widget *widget) noexcept;

  struct Full {};

  /**
   * Create a dialog, but do not associate it with a #Widget yet.
   * Call FinishPreliminary() to resume building the dialog.
   */
  WidgetDialog(Full, UI::SingleWindow &parent, const DialogLook &look,
               const TCHAR *caption) noexcept;

  /**
   * Create a full-screen dialog.
   */
  WidgetDialog(Full, UI::SingleWindow &parent, const DialogLook &look,
               const TCHAR *caption, Widget *widget) noexcept;

  virtual ~WidgetDialog();

  const ButtonLook &GetButtonLook() const {
    return buttons.GetLook();
  }

  void FinishPreliminary(Widget *widget);
  void FinishPreliminary(std::unique_ptr<Widget> widget) noexcept;

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

  Button *AddButton(std::unique_ptr<ButtonRenderer> &&renderer,
                    Button::Callback callback) noexcept {
    return buttons.Add(std::move(renderer), std::move(callback));
  }

  Button *AddButton(const TCHAR *caption,
                    Button::Callback callback) noexcept {
    return buttons.Add(caption, std::move(callback));
  }

  Button *AddButton(const TCHAR *caption, int modal_result) {
    return AddButton(caption, MakeModalResultCallback(modal_result));
  }

  Button *AddSymbolButton(const TCHAR *caption,
                          Button::Callback callback) noexcept {
    return buttons.AddSymbol(caption, std::move(callback));
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

  /* virtual methods from class WndForm */
  void SetModalResult(int id) noexcept override;

private:
  void AutoSize();

protected:
  /* virtual methods from class Window */
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;

  /* virtual methods from class WndForm */
  void ReinitialiseLayout(const PixelRect &parent_rc) noexcept override;
  void SetDefaultFocus() noexcept override;
  bool OnAnyKeyDown(unsigned key_code) noexcept override;
};

/**
 * Helper class for #WidgetDialog which can construct a #Widget of the
 * given type.
 */
template<typename T>
class TWidgetDialog final : public WidgetDialog {
  static_assert(std::is_base_of_v<Widget, T>, "Not a Widget");

public:
  using WidgetDialog::WidgetDialog;

  template<typename... Args>
  void SetWidget(Args&&... args) {
    FinishPreliminary(std::make_unique<T>(std::forward<Args>(args)...));
  }

  auto &GetWidget() noexcept {
    return static_cast<T &>(WidgetDialog::GetWidget());
  }
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
DefaultWidgetDialog(UI::SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, const PixelRect &rc, Widget &widget);

bool
DefaultWidgetDialog(UI::SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, Widget &widget);
