// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ComboList;
class DataField;

int
ComboPicker(const char *caption,
            const ComboList &combo_list,
            const char *help_text = nullptr,
            bool enable_item_help = false,
            const char *extra_caption = nullptr,
            const char *extra_caption2 = nullptr);

/**
 * @return true if the user has selected a new value (though it may be
 * equal to the old one)
 *
 * When #extra_caption is set and the user presses that button, returns
 * false and sets #extra_selected to true (if non-null). Cancel leaves
 * #extra_selected false.
 */
bool
ComboPicker(const char *caption, DataField &df,
            const char *help_text = nullptr,
            const char *extra_caption = nullptr,
            bool *extra_selected = nullptr);
