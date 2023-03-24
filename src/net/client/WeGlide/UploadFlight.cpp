// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UploadFlight.hpp"
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

#include <cinttypes>

namespace WeGlide {

static CurlMime
MakeUploadFlightMime(CURL *easy, const WeGlideSettings &settings,
                     uint_least32_t glider_type,
                     Path igc_path)
{
  CurlMime mime{easy};
  mime.Add("file").Filename("igc_file").FileData(NarrowPathName{igc_path});

  char buffer[32];
  sprintf(buffer, "%u", settings.pilot_id);
  mime.Add("user_id").Data(buffer);
  FormatISO8601(buffer, settings.pilot_birthdate);
  mime.Add("date_of_birth").Data(buffer);
  sprintf(buffer, "%" PRIuLEAST32, glider_type);
  mime.Add("aircraft_id").Data(buffer);

  return mime;
}

Co::Task<boost::json::value>
UploadFlight(CurlGlobal &curl, const WeGlideSettings &settings,
             uint_least32_t glider_type,
             Path igc_path,
             ProgressListener &progress)
{
  NarrowString<0x200> url(settings.default_url);
  url += "/igcfile";

  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  easy.SetFailOnError();

  const auto mime = MakeUploadFlightMime(easy.Get(), settings,
                                         glider_type, igc_path);
  easy.SetMimePost(mime.get());

  Json::ParserOutputStream parser;
  const auto response =
    co_await Curl::CoStreamRequest(curl, std::move(easy), parser);
  co_return parser.Finish();
}

} // namespace WeGlide
