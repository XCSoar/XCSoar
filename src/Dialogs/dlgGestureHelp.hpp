// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Markdown text describing all supported map gestures.
 * Shared between the standalone gesture help dialog and the
 * onboarding wizard.
 */
extern const char gesture_help_text[];

/**
 * Show the gesture help dialog as a standalone modal dialog.
 * Can be called from the Info menu or via xcsoar://dialog/gestures.
 */
void
dlgGestureHelpShowModal() noexcept;
