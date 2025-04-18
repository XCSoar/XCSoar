// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class ListItemRenderer;

/** returns string of item's help text **/
typedef const TCHAR* (*ItemHelpCallback_t)(unsigned item);

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
