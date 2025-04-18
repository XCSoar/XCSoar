// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class DataField;

/**
 * Show a dialog to edit the value of a #DataField.
 *
 * @return true if the value has been modified
 */
bool
EditDataFieldDialog(const TCHAR *caption, DataField &df,
                    const TCHAR *help_text);
