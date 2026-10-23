// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Account.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

#include <chrono>

namespace SoaringScoring {
namespace {

uint32_t
UnixNow() noexcept
{
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
  if (seconds <= 0)
    return 0;
  if (static_cast<uint64_t>(seconds) > 0xffffffffu)
    return 0xffffffffu;
  return static_cast<uint32_t>(seconds);
}

} // namespace

AccountToken
LoadAccountToken() noexcept
{
  AccountToken account;
  Profile::Get(ProfileKeys::SoaringScoringAccessToken,
               account.access_token);
  unsigned expiry = 0;
  if (Profile::Get(ProfileKeys::SoaringScoringAccessExpiry, expiry))
    account.access_expires_at = expiry;
  return account;
}

void
SaveAccountToken(const AccountToken &account) noexcept
{
  Profile::Set(ProfileKeys::SoaringScoringAccessToken,
               account.access_token);
  Profile::Set(ProfileKeys::SoaringScoringAccessExpiry,
               account.access_expires_at);
  Profile::Save();
}

void
ClearAccountToken() noexcept
{
  SaveAccountToken({});
}

bool
AccountConnected() noexcept
{
  const auto account = LoadAccountToken();
  if (account.access_token.empty())
    return false;
  if (account.access_expires_at == 0)
    return true;
  return UnixNow() < account.access_expires_at;
}

} // namespace SoaringScoring
