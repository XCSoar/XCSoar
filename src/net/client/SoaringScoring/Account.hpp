// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>
#include <string>

namespace SoaringScoring {

struct AccountToken {
  std::string access_token;
  uint32_t access_expires_at = 0;
};

[[nodiscard]] AccountToken
LoadAccountToken() noexcept;

void
SaveAccountToken(const AccountToken &account) noexcept;

void
ClearAccountToken() noexcept;

/** True when a token is stored and its expiry, if set, is still ahead. */
[[nodiscard]] bool
AccountConnected() noexcept;

/** True when the key is absent. The Services page can turn it off. */
[[nodiscard]] bool
AutomaticUpload() noexcept;

} // namespace SoaringScoring
