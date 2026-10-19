// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Run the WeGlide device login.
 *
 * Contacts the server, then shows the QR code and the browser link
 * until the pilot approves, denies, or closes the dialog. A granted
 * token is stored in the profile.
 *
 * @return true when a token was stored
 */
bool
RunWeGlideDeviceSignIn() noexcept;

/**
 * True while the sign-in dialog is polling `/v1/auth/token`.
 * Other callers leave that endpoint alone until this is false.
 */
[[nodiscard]] bool
IsWeGlideSignInOpen() noexcept;

/** True when a WeGlide access token is stored. */
[[nodiscard]] bool
WeGlideAccountConnected() noexcept;

/** Drop the stored WeGlide access and refresh tokens. */
void
ForgetWeGlideAccount() noexcept;
