// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Run the SoaringScoring device login.
 *
 * Shows the user code and the approval link until the pilot approves,
 * denies, or closes the dialog. A granted token is stored in the
 * profile.
 *
 * @return true when a token was stored
 */
bool
RunSoaringScoringDeviceSignIn() noexcept;
