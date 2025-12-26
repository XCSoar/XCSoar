// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

namespace UI { class SingleWindow; }

/**
 * Show a dialog with the current NOTAM list and reload functionality.
 *
 * @param parent the parent window
 */
void
ShowNOTAMListDialog(UI::SingleWindow &parent);