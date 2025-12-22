// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct InfoBoxPanel;

void
dlgInfoBoxAccessShowModeless(int id, const InfoBoxPanel *panels);

/**
 * Close the currently open InfoBox dialog if one exists.
 */
void
dlgInfoBoxAccessClose() noexcept;

/**
 * Close the currently open InfoBox dialog if it's not owned by the specified InfoBox.
 * @param id The InfoBox ID that should keep its dialog open.
 * @return true if a dialog was closed (different InfoBox owned it)
 */
bool
dlgInfoBoxAccessCloseOthers(int id) noexcept;
