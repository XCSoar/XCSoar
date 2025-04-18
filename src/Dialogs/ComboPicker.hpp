// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class ComboList;
class DataField;

int
ComboPicker(const TCHAR *caption,
            const ComboList &combo_list,
            const TCHAR *help_text = nullptr,
            bool enable_item_help = false,
            const TCHAR *extra_caption=nullptr);

/**
 * @return true if the user has selected a new value (though it may be
 * equal to the old one)
 */
bool
ComboPicker(const TCHAR *caption, DataField &df,
            const TCHAR *help_text = nullptr);
