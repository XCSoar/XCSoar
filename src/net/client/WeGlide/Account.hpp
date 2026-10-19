// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

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
  /**
   * No usable access token. A failed refresh clears the refresh
   * token, and an expired access token is cleared too.
   */
  FAILED,
};

/** The sign-in dialog is polling the token endpoint. */
void
SetSignInOpen(bool open) noexcept;

[[nodiscard]] bool
IsSignInOpen() noexcept;

[[nodiscard]] AccountToken
LoadAccountToken() noexcept;

void
SaveAccountToken(const AccountToken &account) noexcept;

/**
 * Save a refresh result only when this operation still owns the
 * refresh token. A newer sign-in or refresh must not be overwritten.
 * An unchanged result is not written back.
 */
[[nodiscard]] constexpr bool
ShouldSaveRefreshedAccount(RefreshOutcome outcome,
                           std::string_view profile_refresh,
                           std::string_view refresh_at_start) noexcept
{
  return outcome != RefreshOutcome::UNCHANGED &&
         profile_refresh == refresh_at_start;
}

/**
 * The access token can still be sent. An unset expiry is treated as
 * usable. @p now is Unix seconds.
 */
[[nodiscard]] constexpr bool
AccessTokenUsableAt(const AccountToken &account, uint32_t now) noexcept
{
  return !account.access_token.empty() &&
         (account.access_expires_at == 0 || now < account.access_expires_at);
}

[[nodiscard]] bool
AccessTokenUsable(const AccountToken &account) noexcept;

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
