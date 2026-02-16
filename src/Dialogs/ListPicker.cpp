// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/ListPicker.hpp"
#include "HelpDialog.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/StaticHelpTextWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "WidgetDialog.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "ui/event/Timer.hpp"

#include <cassert>

int
ListPicker(const char *caption,
           unsigned num_items, unsigned initial_value,
           unsigned item_height,
           ListItemRenderer &item_renderer, bool update,
           const char *help_text,
           ItemHelpCallback_t _itemhelp_callback,
           const char *extra_caption)
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
  } else if (help_text != nullptr) {
    /* show static help text at the bottom instead of behind a
       Help button */
    widget = std::make_unique<StaticHelpTextWidget>(std::move(widget),
                                                    help_text);
  }

  if (num_items > 0)
    dialog.AddButton(_("Select"), mrOK);

  if (extra_caption != nullptr)
    dialog.AddButton(extra_caption, -2);

  /* only show a Help button when item help is active (the general
     help text complements the per-item help); for pickers without
     item help, the help text is already shown at the bottom */
  if (help_text != nullptr && _itemhelp_callback != nullptr)
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
