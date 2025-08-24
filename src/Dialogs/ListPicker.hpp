// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Form.hpp"
#include "HelpDialog.hpp"
#include "UIGlobals.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"

#include <tchar.h>

class ListItemRenderer;

/** returns string of item's help text **/
typedef const TCHAR* (*ItemHelpCallback_t)(unsigned item);

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
  UI::Timer postpone_update_help{
      [this] { UpdateHelp(GetList().GetCursorIndex()); }};

  const TCHAR *const caption, *const help_text;
  ItemHelpCallback_t item_help_callback;
  TextWidget *help_widget;
  TwoWidgets *two_widgets;

public:
  ListPickerWidget(unsigned _num_items, unsigned _initial_value,
                   unsigned _row_height, ListItemRenderer &_item_renderer,
                   WndForm &_dialog, const TCHAR *_caption,
                   const TCHAR *_help_text) noexcept
      : num_items(_num_items),
        initial_value(_initial_value),
        row_height(_row_height),
        visible(false),
        item_renderer(_item_renderer),
        dialog(_dialog),
        caption(_caption),
        help_text(_help_text),
        item_help_callback(nullptr)
  {
  }

  using ListWidget::GetList;

  void EnableItemHelp(ItemHelpCallback_t _item_help_callback,
                      TextWidget &_help_widget,
                      TwoWidgets &_two_widgets) noexcept
  {
    item_help_callback = _item_help_callback;
    help_widget = &_help_widget;
    two_widgets = &_two_widgets;
  }

  void UpdateHelp(unsigned index) noexcept
  {
    if (!visible || item_help_callback == nullptr) return;

    help_widget->SetText(item_help_callback(index));
    two_widgets->UpdateLayout();
  }

  void ShowHelp() noexcept { HelpDialog(caption, help_text); }

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    ListControl &list =
        CreateList(parent, UIGlobals::GetDialogLook(), rc, row_height);
    list.SetLength(num_items);
    list.SetCursorIndex(initial_value);
  }

  void Show(const PixelRect &rc) noexcept override
  {
    ListWidget::Show(rc);

    visible = true;
    postpone_update_help.Schedule({});
  }

  void Hide() noexcept override
  {
    visible = false;
    postpone_update_help.Cancel();
    ListWidget::Hide();
  }

  /* virtual methods from class ListControl::Handler */

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override
  {
    item_renderer.OnPaintItem(canvas, rc, idx);
  }

  void OnCursorMoved(unsigned index) noexcept override { UpdateHelp(index); }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override
  {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override
  {
    dialog.SetModalResult(mrOK);
  }
};

/**
 * Shows a list dialog and lets the user pick an item.
 * @param caption
 * @param num_items
 * @param initial_value
 * @param item_height
 * @param item_renderer Paint a single item
 * @param update Update per timer
 * @param help_text enable the "Help" button and show this text on click
 * @param itemhelp_callback Callback to return string for current item help
 * @param extra_caption caption of another button that closes the
 * dialog (nullptr disables it)
 * @return the list index, -1 if the user cancelled the dialog, -2 if
 * the user clicked the "extra" button
 */
int
ListPicker(const TCHAR *caption,
           unsigned num_items, unsigned initial_value,
           unsigned item_height,
           ListItemRenderer &item_renderer, bool update = false,
           const TCHAR *help_text = nullptr,
           ItemHelpCallback_t itemhelp_callback = nullptr,
           const TCHAR *extra_caption=nullptr);
