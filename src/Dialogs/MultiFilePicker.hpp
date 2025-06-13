// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifndef XCSOAR_DIALOGS_MULTI_FILE_PICKER_HPP
#define XCSOAR_DIALOGS_MULTI_FILE_PICKER_HPP

#include <tchar.h>

class MultiFileDataField;

bool MultiFilePicker(const TCHAR *caption, MultiFileDataField &df,
                     const TCHAR *help_text = nullptr);

#endif
