// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Client.hpp"
#include "Protocol.hpp"
#include "Settings.hpp"

#include "json/ParserOutputStream.hxx"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Setup.hxx"
#include "lib/curl/Slist.hxx"
#include "co/Task.hxx"

#include <stdexcept>

namespace PureTrack {

Co::Task<void>
Client::Insert(const Settings &settings, const Sample &sample)
{
  if (settings.endpoint.empty())
    throw std::invalid_argument("PureTrack endpoint missing");

  if (settings.app_key.empty())
    throw std::invalid_argument("PureTrack app key missing");

  if (settings.device_id.empty())
    throw std::invalid_argument("PureTrack device id missing");

  const auto body_string = BuildInsertRequestBody(settings, sample);

  CurlEasy easy{settings.endpoint.c_str()};
  Curl::Setup(easy);
  easy.SetPost();
  easy.SetTimeout(25);

  CurlSlist headers;
  headers.Append("Accept: application/json");
  headers.Append("Content-Type: application/json");
  easy.SetRequestHeaders(headers.Get());
  easy.SetRequestBody(body_string);

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  const auto response_body = parser.Finish();

  if (response.status != 200)
    throw ResponseToException(response.status, response_body);

  if (!ParseInsertResponse(response.status, response_body).success)
    throw ResponseToException(response.status, response_body);

  co_return;
}

} // namespace PureTrack
