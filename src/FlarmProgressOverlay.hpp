// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A lightweight progress bar overlay shown at the bottom of the
 * screen when the FLARM reports a long-running operation (firmware
 * update, obstacle database upload, IGC download, etc.) via PFLAQ.
 */
namespace FlarmProgressOverlay {

void Show(const char *text, unsigned progress) noexcept;
void Close() noexcept;

} // namespace FlarmProgressOverlay
