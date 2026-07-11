// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Form/Form.hpp"
#include "HelpDialog.hpp"
#include "UIGlobals.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Widget/ListWidget.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"

class ListItemRenderer;

/** returns string of item's help text **/
typedef const char* (*ItemHelpCallback_t)(unsigned item);

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

  const char *const caption, *const help_text;
  ItemHelpCallback_t item_help_callback;
  TextWidget *help_widget;
  TwoWidgets *two_widgets;

  /** When the list is empty but an extra action (e.g. Download) exists. */
  bool empty_extra_action;
  TextRowRenderer empty_row_renderer;

public:
  ListPickerWidget(unsigned _num_items, unsigned _initial_value,
                   unsigned _row_height, ListItemRenderer &_item_renderer,
                   WndForm &_dialog, const char *_caption,
                   const char *_help_text, bool _empty_extra_action) noexcept
      : num_items(_num_items),
        initial_value(_initial_value),
        row_height(_row_height),
        visible(false),
        item_renderer(_item_renderer),
        dialog(_dialog),
        caption(_caption),
        help_text(_help_text),
        item_help_callback(nullptr),
        empty_extra_action(_empty_extra_action)
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

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

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

  unsigned OnListResized() noexcept override;

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  void OnCursorMoved(unsigned index) noexcept override { UpdateHelp(index); }

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override
  {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override
  {
    if (num_items == 0 && empty_extra_action)
      dialog.SetModalResult(mrExtra);
    else
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
 * @param help_text if no itemhelp_callback, show this text at the bottom;
 * otherwise enable the "Help" button and show this text on click
 * @param itemhelp_callback Callback to return string for current item help
 * @param extra_caption caption of another button that closes the
 * dialog (nullptr disables it)
 * @param extra_caption2 caption of a second extra button that closes the
 * dialog (nullptr disables it)
 * @return the list index, -1 if the user cancelled the dialog, mrExtra if
 * the user clicked the first extra button, mrExtra2 for the second
 */
int
ListPicker(const char *caption,
           unsigned num_items, unsigned initial_value,
           unsigned item_height,
           ListItemRenderer &item_renderer, bool update = false,
           const char *help_text = nullptr,
           ItemHelpCallback_t itemhelp_callback = nullptr,
           const char *extra_caption = nullptr,
           const char *extra_caption2 = nullptr);
