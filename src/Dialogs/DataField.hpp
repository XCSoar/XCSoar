// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class DataField;

/**
 * Show a dialog to edit the value of a #DataField.
 *
 * @return true if the value has been modified
 */
bool
EditDataFieldDialog(const char *caption, DataField &df,
                    const char *help_text);

/**
 * Edit a non-negative #DataFieldFloat as a whole number on the digit
 * pad.  The value stays in the field's own unit (feet or metres).
 *
 * @return true if the user confirmed the dialog
 */
bool
EditUnsignedFloatDialog(const char *caption, DataField &df,
                        const char *help_text);
