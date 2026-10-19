// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Account.hpp"
#include "Auth.hpp"
#include "Dialogs/Settings/WeGlideDeviceSignIn.hpp"
#include "Profile/Profile.hpp"
#include "co/Task.hxx"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"

#include <chrono>

namespace WeGlide {
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
  Profile::Get(ProfileKeys::WeGlideAccessToken, account.access_token);
  Profile::Get(ProfileKeys::WeGlideRefreshToken, account.refresh_token);
  unsigned expiry = 0;
  if (Profile::Get(ProfileKeys::WeGlideAccessExpiry, expiry))
    account.access_expires_at = expiry;
  return account;
}

void
SaveAccountToken(const AccountToken &account) noexcept
{
  Profile::Set(ProfileKeys::WeGlideAccessToken, account.access_token);
  Profile::Set(ProfileKeys::WeGlideRefreshToken, account.refresh_token);
  Profile::Set(ProfileKeys::WeGlideAccessExpiry, account.access_expires_at);
  Profile::Save();
}

Co::Task<RefreshOutcome>
RefreshAccount(CurlGlobal &curl, AccountToken &account)
{
  if (IsWeGlideSignInOpen())
    co_return RefreshOutcome::UNCHANGED;

  const uint32_t now = UnixNow();
  Net::Client::Auth::DeviceFlow flow;
  flow.token.access_token = account.access_token;
  flow.token.refresh_token = account.refresh_token;
  flow.access_expires_at = account.access_expires_at;

  using Net::Client::Auth::DeviceFlowAction;
  if (flow.Next(now, kAccessRefreshMargin) != DeviceFlowAction::REFRESH) {
    co_return account.access_token.empty()
      ? RefreshOutcome::FAILED
      : RefreshOutcome::UNCHANGED;
  }

  const auto form = Net::Client::Auth::RefreshForm(kOAuthClientId,
                                                   account.refresh_token);
  const std::string url{kTokenUrl};
  CurlEasy easy{url.c_str()};
  Curl::Setup(easy);
  easy.SetTimeout(30);
  easy.SetPost();
  easy.SetRequestBody(form);

  CurlSlist headers;
  headers.Append("Content-Type: application/x-www-form-urlencoded");
  easy.SetRequestHeaders(headers.Get());

  const auto response = co_await Curl::CoRequest(curl, std::move(easy));
  const auto action = flow.OnRefresh(response.status, response.body,
                                     UnixNow());
  account.access_token = std::move(flow.token.access_token);
  account.refresh_token = std::move(flow.token.refresh_token);
  account.access_expires_at = flow.access_expires_at;
  co_return action == DeviceFlowAction::READY
    ? RefreshOutcome::UPDATED
    : RefreshOutcome::FAILED;
}

} // namespace WeGlide
