// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Show the disclaimer dialog if the user hasn't acknowledged it
 * for the current version.
 *
 * @return true if the disclaimer was acknowledged (or was already
 *         acknowledged), false if the user declined
 */
bool
CheckShowDisclaimer() noexcept;

/**
 * Show the disclaimer dialog unconditionally.
 *
 * @return true if the user acknowledged, false if declined
 */
bool
dlgDisclaimerShowModal() noexcept;
