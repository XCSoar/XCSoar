// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/ListPicker.hpp"
#include "HelpDialog.hpp"
#include "WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "ui/event/Timer.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <cassert>

class ListPickerWidget : public ListWidget {
  unsigned num_items;
  unsigned initial_value;
  unsigned row_height;

  bool visible;

  ListItemRenderer &item_renderer;
  WndForm &dialog;

  /**
   * This timer is used to postpone the initial UpdateHelp() call.
   * This is necessary because the TwoWidgets instance is not fully
   * initialised yet in Show(), and recursively calling into Widget
   * methods is dangerous anyway.
   */
  UI::Timer postpone_update_help{[this]{
    UpdateHelp(GetList().GetCursorIndex());
  }};

  const TCHAR *const caption, *const help_text;
  ItemHelpCallback_t item_help_callback;
  TextWidget *help_widget;
  TwoWidgets *two_widgets;

public:
  ListPickerWidget(unsigned _num_items, unsigned _initial_value,
                   unsigned _row_height,
                   ListItemRenderer &_item_renderer,
                   WndForm &_dialog,
                   const TCHAR *_caption, const TCHAR *_help_text) noexcept
    :num_items(_num_items), initial_value(_initial_value),
     row_height(_row_height),
     visible(false),
     item_renderer(_item_renderer),
     dialog(_dialog),
     caption(_caption), help_text(_help_text),
     item_help_callback(nullptr) {}

  using ListWidget::GetList;

  void EnableItemHelp(ItemHelpCallback_t _item_help_callback,
                      TextWidget &_help_widget,
                      TwoWidgets &_two_widgets) noexcept {
    item_help_callback = _item_help_callback;
    help_widget = &_help_widget;
    two_widgets = &_two_widgets;
  }

  void UpdateHelp(unsigned index) noexcept {
    if (!visible || item_help_callback == nullptr)
      return;

    help_widget->SetText(item_help_callback(index));
    two_widgets->UpdateLayout();
  }

  void ShowHelp() noexcept {
    HelpDialog(caption, help_text);
  }

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    ListControl &list = CreateList(parent, UIGlobals::GetDialogLook(), rc,
                                   row_height);
    list.SetLength(num_items);
    list.SetCursorIndex(initial_value);
  }

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);

    visible = true;
    postpone_update_help.Schedule({});
  }

  void Hide() noexcept override {
    visible = false;
    postpone_update_help.Cancel();
    ListWidget::Hide();
  }

  /* virtual methods from class ListControl::Handler */

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    item_renderer.OnPaintItem(canvas, rc, idx);
  }

  void OnCursorMoved(unsigned index) noexcept override {
    UpdateHelp(index);
  }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    dialog.SetModalResult(mrOK);
  }
};

int
ListPicker(const TCHAR *caption,
           unsigned num_items, unsigned initial_value,
           unsigned item_height,
           ListItemRenderer &item_renderer, bool update,
           const TCHAR *help_text,
           ItemHelpCallback_t _itemhelp_callback,
           const TCHAR *extra_caption)
{
  assert(num_items <= 0x7fffffff);
  assert((num_items == 0 && initial_value == 0) || initial_value < num_items);
  assert(item_height > 0);

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), caption);

  ListPickerWidget *const list_widget =
    new ListPickerWidget(num_items, initial_value, item_height,
                         item_renderer, dialog, caption, help_text);

  std::unique_ptr<Widget> widget(list_widget);

  if (_itemhelp_callback != nullptr) {
    widget = std::make_unique<TwoWidgets>(std::move(widget),
                                          std::make_unique<TextWidget>());
    auto &two_widgets = (TwoWidgets &)*widget;
    list_widget->EnableItemHelp(_itemhelp_callback,
                                (TextWidget &)two_widgets.GetSecond(),
                                two_widgets);
  }

  if (num_items > 0)
    dialog.AddButton(_("Select"), mrOK);

  if (extra_caption != nullptr)
    dialog.AddButton(extra_caption, -2);

  if (help_text != nullptr)
    dialog.AddButton(_("Help"), [list_widget](){
      list_widget->ShowHelp();
    });

  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.EnableCursorSelection();

  UI::PeriodicTimer update_timer([list_widget](){
    list_widget->GetList().Invalidate();
  });
  if (update)
    update_timer.Schedule(std::chrono::seconds(1));

  dialog.FinishPreliminary(widget.release());

  int result = dialog.ShowModal();
  if (result == mrOK)
    result = (int)list_widget->GetList().GetCursorIndex();
  else if (result != -2)
    result = -1;

  return result;
}
