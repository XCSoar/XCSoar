// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Show the unified onboarding dialog.
 *
 * Dynamically assembles pages based on which conditions have already
 * been met (warranty acknowledged, permissions granted, cloud
 * preference set, informational pages suppressed).
 *
 * @param force_info If true, always show the informational pages
 *                   regardless of the "don't show again" setting.
 *                   Used when invoked from the Info menu.
 * @return true if the user completed the dialog (or it was skipped
 *         because all conditions are met), false if the user declined
 *         the warranty disclaimer (the app should exit)
 */
bool
dlgOnboardingShowModal(bool force_info = false);
