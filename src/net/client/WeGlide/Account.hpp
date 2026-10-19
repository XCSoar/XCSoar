// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>

class CurlGlobal;

namespace Co { template<typename T> class Task; }

namespace WeGlide {

/** Bearer token stored in the profile. Not copied into settings. */
struct AccountToken {
  std::string access_token;
  std::string refresh_token;
  uint32_t access_expires_at = 0;
};

enum class RefreshOutcome : uint8_t {
  /** Still valid, or the sign-in dialog already owns the token endpoint. */
  UNCHANGED,
  /** A new access token was stored in @p account. The caller saves it. */
  UPDATED,
  /** No usable access token. A failed refresh clears the refresh token. */
  FAILED,
};

[[nodiscard]] AccountToken
LoadAccountToken() noexcept;

void
SaveAccountToken(const AccountToken &account) noexcept;

/**
 * One refresh POST when the access token is inside the margin.
 *
 * Runs on the network thread. The caller saves @p account on the UI
 * thread after this returns. Does nothing while the sign-in dialog
 * is open.
 */
[[nodiscard]] Co::Task<RefreshOutcome>
RefreshAccount(CurlGlobal &curl, AccountToken &account);

} // namespace WeGlide
