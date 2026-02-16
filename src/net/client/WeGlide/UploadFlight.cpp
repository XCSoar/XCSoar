// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UploadFlight.hpp"
#include "Error.hpp"
#include "Settings.hpp"
#include "net/http/Progress.hpp"
#include "lib/curl/CoStreamRequest.hxx"
#include "lib/curl/Easy.hxx"
#include "lib/curl/Mime.hxx"
#include "lib/curl/Setup.hxx"
#include "Formatter/TimeFormatter.hpp"
#include "json/ParserOutputStream.hxx"
#include "system/ConvertPathName.hpp"
#include "system/Path.hpp"
#include "util/StaticString.hxx"

#include <fmt/format.h>

namespace WeGlide {

static CurlMime
MakeUploadFlightMime(CURL *easy, const WeGlideSettings &settings,
                     uint_least32_t glider_type,
                     Path igc_path)
{
  CurlMime mime{easy};
  mime.Add("file").Filename("igc_file").FileData(NarrowPathName{igc_path});

  char buffer[32];
  mime.Add("user_id").Data(fmt::format_int{settings.pilot_id}.c_str());
  FormatISO8601(buffer, settings.pilot_birthdate);
  mime.Add("date_of_birth").Data(buffer);
  mime.Add("aircraft_id").Data(fmt::format_int{glider_type}.c_str());

  return mime;
}

Co::Task<boost::json::value>
UploadFlight(CurlGlobal &curl, const WeGlideSettings &settings,
             uint_least32_t glider_type,
             Path igc_path,
             ProgressListener &progress)
{
  StaticString<0x200> url(settings.default_url);
  url += "/igcfile";

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};

  const auto mime = MakeUploadFlightMime(easy.Get(), settings,
                                         glider_type, igc_path);
  easy.SetMimePost(mime.get());

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  auto body = parser.Finish();

  if (response.status != 201)
    throw ResponseToException(response.status, body);

  co_return body;
}

} // namespace WeGlide
