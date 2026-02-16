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
            const char *extra_caption=nullptr);

/**
 * @return true if the user has selected a new value (though it may be
 * equal to the old one)
 */
bool
ComboPicker(const char *caption, DataField &df,
            const char *help_text = nullptr);
