/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/ListPicker.hpp"
#include "WidgetDialog.hpp"
#include "Form/ListWidget.hpp"
#include "Form/TextWidget.hpp"
#include "Form/TwoWidgets.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Event/FunctionalTimer.hpp"
#include "Event/Timer.hpp"

#include <assert.h>

static constexpr int HELP = 100;

class ListPickerWidget : public ListWidget, public ActionListener,
                         private Timer {
  unsigned num_items;
  unsigned initial_value;
  UPixelScalar row_height;

  bool visible;

  ListControl::PaintItemCallback paint_callback;
  ActionListener &action_listener;

  ListHelpCallback_t help_callback;
  ItemHelpCallback_t item_help_callback;
  TextWidget *help_widget;
  TwoWidgets *two_widgets;

public:
  ListPickerWidget(unsigned _num_items, unsigned _initial_value,
                   UPixelScalar _row_height,
                   ListControl::PaintItemCallback _paint_callback,
                   ActionListener &_action_listener,
                   ListHelpCallback_t _help_callback)
    :num_items(_num_items), initial_value(_initial_value),
     row_height(_row_height),
     visible(false),
     paint_callback(_paint_callback),
     action_listener(_action_listener),
     help_callback(_help_callback),
     item_help_callback(nullptr) {}

  using ListWidget::GetList;

  void EnableItemHelp(ItemHelpCallback_t _item_help_callback,
                      TextWidget *_help_widget,
                      TwoWidgets *_two_widgets) {
    item_help_callback = _item_help_callback;
    help_widget = _help_widget;
    two_widgets = _two_widgets;
  }

  void UpdateHelp(unsigned index) {
    if (!visible || item_help_callback == nullptr)
      return;

    help_widget->SetText(item_help_callback(index));
    two_widgets->UpdateLayout();
  }

  /* virtual methods from class Widget */

  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) gcc_override {
    ListControl &list = CreateList(parent, UIGlobals::GetDialogLook(), rc,
                                   row_height);
    list.SetLength(num_items);
    list.SetCursorIndex(initial_value);
  }

  virtual void Unprepare() gcc_override {
    DeleteWindow();
  }

  virtual void Show(const PixelRect &rc) gcc_override {
    ListWidget::Show(rc);

    visible = true;
    Schedule(0);
  }

  virtual void Hide() gcc_override {
    visible = false;
    Cancel();
    ListWidget::Hide();
  }

  /* virtual methods from class ListControl::Handler */

  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) gcc_override {
    paint_callback(canvas, rc, idx);
  }

  virtual void OnCursorMoved(unsigned index) gcc_override {
    UpdateHelp(index);
  }

  virtual bool CanActivateItem(unsigned index) const gcc_override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) gcc_override {
    action_listener.OnAction(mrOK);
  }

  /* virtual methods from class ActionListener */

  virtual void OnAction(int id) gcc_override {
    help_callback(GetList().GetCursorIndex());
  }

private:
  /* virtual methods from class Timer */

  /**
   * This timer is used to postpone the initial UpdateHelp() call.
   * This is necessary because the TwoWidgets instance is not fully
   * initialised yet in Show(), and recursively calling into Widget
   * methods is dangerous anyway.
   */
  virtual void OnTimer() gcc_override {
    UpdateHelp(GetList().GetCursorIndex());
  }
};

int
ListPicker(SingleWindow &parent, const TCHAR *caption,
           unsigned num_items, unsigned initial_value,
           UPixelScalar item_height,
           ListControl::PaintItemCallback paint_callback, bool update,
           ListHelpCallback_t _help_callback,
           ItemHelpCallback_t _itemhelp_callback)
{
  assert(num_items <= 0x7fffffff);
  assert((num_items == 0 && initial_value == 0) || initial_value < num_items);
  assert(item_height > 0);
  assert(paint_callback != NULL);

  WidgetDialog dialog(UIGlobals::GetDialogLook());

  ListPickerWidget *const list_widget =
    new ListPickerWidget(num_items, initial_value, item_height,
                         paint_callback, dialog, _help_callback);
  TextWidget *text_widget = nullptr;
  TwoWidgets *two_widgets = nullptr;

  Widget *widget = list_widget;

  if (_itemhelp_callback != nullptr) {
    text_widget = new TextWidget();
    widget = two_widgets = new TwoWidgets(list_widget, text_widget);

    list_widget->EnableItemHelp(_itemhelp_callback, text_widget, two_widgets);
  }

  dialog.CreateFull(parent, caption, widget);

  if (_help_callback != nullptr)
    dialog.AddButton(_("Help"), list_widget, HELP);

  if (num_items > 0)
    dialog.AddButton(_("Select"), mrOK);

  dialog.AddButton(_("Cancel"), mrCancel);

  FunctionalTimer update_timer;
  if (update)
    update_timer.Schedule([list_widget]() {
        list_widget->GetList().Invalidate();
      }, 1000);

  return dialog.ShowModal() == mrOK
    ? (int)list_widget->GetList().GetCursorIndex()
    : -1;
}
