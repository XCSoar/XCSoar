// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/fmt/RuntimeError.hxx"
#include "util/NumberParser.hxx"
#include "co/Task.hxx"

#include <stdexcept>

namespace LiveTrack24 {

static constexpr const char *BASE_URL = "https://t2.livetrack24.com";

Co::Task<std::string>
Client::Request(std::string path)
{
  CurlEasy easy{(BASE_URL + path).c_str()};
  Curl::Setup(easy);
  easy.SetFailOnError();

  const auto response = co_await Curl::CoRequest(curl, std::move(easy));

  const auto reply = ParseReply(response.body);
  if (!reply)
    throw std::runtime_error("Unexpected reply from LiveTrack24");

  if (!reply->IsOK())
    throw FmtRuntimeError("LiveTrack24: {} ({})", reply->message,
                          reply->code);

  co_return std::string{reply->data};
}

Co::Task<UserID>
Client::GetUserID(const char *username, const char *password,
                  std::string_view app_secret)
{
  /* login uses session id 0 */
  const std::string data =
    co_await Request(LoginPath(app, MakeToken(password, 0, app_secret),
                               username));

  const auto user_id = ParseInteger<UserID>(data);
  if (!user_id || *user_id == 0)
    throw std::runtime_error("LiveTrack24 sent no user id");

  co_return *user_id;
}

Co::Task<void>
Client::SendPosition(const Session &session, unsigned packet_id,
                     const Fix &fix, const TrackInfo *info)
{
  /* the path is built before the coroutine starts, so the
     references do not have to outlive this call */
  return [](Client &client, std::string path) -> Co::Task<void> {
    co_await client.Request(std::move(path));
  }(*this, TrackPath(app, session, packet_id, fix, info));
}

Co::Task<void>
Client::EndTracking(const Session &session, unsigned packet_id)
{
  return [](Client &client, std::string path) -> Co::Task<void> {
    co_await client.Request(std::move(path));
  }(*this, TrackEndPath(app, session, packet_id));
}

} // namespace LiveTrack24
