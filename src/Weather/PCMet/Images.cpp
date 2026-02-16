// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Images.hpp"
#include "Settings.hpp"
#include "net/http/CoDownloadToFile.hpp"
#include "net/http/Progress.hpp"
#include "lib/curl/CoRequest.hxx"
#include "lib/curl/Setup.hxx"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "util/ConvertString.hpp"
#include "util/StringSplit.hxx"

#include <stdexcept>

#include <stdio.h>

#define PCMET_URI "https://www.flugwetter.de"

static constexpr PCMet::ImageArea rad_lokal_areas[] = {
  { "pro", "Prötzel" },
  { "drs", "Dresden" },
  { "eis", "Eisberg" },
  { "emd", "Emden" },
  { "ess", "Essen" },
  { "fbg", "Feldberg" },
  { "fld", "Flechtdorf" },
  { "boo", "Boostedt" },
  { "hnr", "Hannover" },
  { "mem", "Memmingen" },
  { "isn", "Isen" },
  { "neu", "Neuhaus" },
  { "nhb", "Neuheilenbach" },
  { "oft", "Offenthal" },
  { "ros", "Rostock" },
  { "tur", "Türkheim" },
  { "umd", "Ummendorf" },
  { "eddb", "EDDB" },
  { "eddc", "EDDC" },
  { "edde", "EDDE" },
  { "eddf", "EDDF" },
  { "eddg", "EDDG" },
  { "eddh", "EDDH" },
  { "eddk", "EDDK" },
  { "eddl", "EDDL" },
  { "eddm", "EDDM" },
  { "eddn", "EDDN" },
  { "eddp", "EDDP" },
  { "eddr", "EDDR" },
  { "edds", "EDDS" },
  { "eddt", "EDDT" },
  { "eddv", "EDDV" },
  { "eddw", "EDDW" },
  { nullptr, nullptr }
};

static constexpr PCMet::ImageArea rad_areas[] = {
  { "de", "Deutschland" },
  { "eu", "Europa" },
  { nullptr, nullptr }
};

static constexpr PCMet::ImageArea sat_areas[] = {
  { "vis_hrv_eu", "Mitteleuropa HRV" },
  { "ir_rgb_eu", "Mitteleuropa RGB" },
  { "ir_108_eu", "Mitteleuropa IR" },
  { "vis_hrv_ce", "Mitteleuropa HRV" },
  { "ir_rgb_ce", "Mitteleuropa RGB" },
  { "ir_108_ce", "Mitteleuropa IR" },
  { "vis_hrv_mdl", "Deutschland HRV" },
  { "ir_rgb_mdl", "Deutschland RGB" },
  { "ir_108_mdl", "Deutschland IR" },
  { "vis_hrv_ndl", "Deutschland Nord HRV" },
  { "ir_rgb_ndl", "Deutschland Nord RGB" },
  { "ir_108_ndl", "Deutschland Nord IR" },
  { "vis_hrv_sdl", "Deutschland Süd HRV" },
  { "ir_rgb_sdl", "Deutschland Süd RGB" },
  { "ir_108_sdl", "Deutschland Süd IR" },
  { nullptr, nullptr }
};

const PCMet::ImageType PCMet::image_types[] = {
  { "rad_lokal/einzelstandorte.htm", "Lokale RADAR-Bilder", rad_lokal_areas },
  { "rad/index.htm", "RADAR", rad_areas },
  { "sat/index.htm", "Satellitenbilder", sat_areas },
  { nullptr, nullptr, nullptr },
};

static Co::Task<Curl::CoResponse>
CoGet(CurlGlobal &curl, const char *url,
      const char *username, const char *password,
      ProgressListener &progress)
{
  CurlEasy easy{url};
  Curl::Setup(easy);
  const Net::ProgressAdapter progress_adapter{easy, progress};
  easy.SetFailOnError();

  if (username != nullptr)
    easy.SetOption(CURLOPT_USERNAME, username);
  if (password != nullptr)
    easy.SetOption(CURLOPT_PASSWORD, password);

  // TODO limit the response body size
  co_return co_await Curl::CoRequest(curl, std::move(easy));
}

Co::Task<AllocatedPath>
PCMet::DownloadLatestImage(const char *type, const char *area,
                           const PCMetSettings &settings,
                           CurlGlobal &curl, ProgressListener &progress)
{
  const WideToUTF8Converter username(settings.www_credentials.username);
  const WideToUTF8Converter password(settings.www_credentials.password);

  char url[256];
  snprintf(url, sizeof(url),
           PCMET_URI "/fw/bilder/%s?type=%s",
           type, area);

  // download the HTML page
  const auto response =
    co_await CoGet(curl, url, username, password, progress);

  static constexpr char img_needle[] = "<img name=\"bild\" src=\"/";
  const char *img = strstr(response.body.c_str(), img_needle);
  if (img == nullptr)
    throw std::runtime_error("No IMG tag found in pc_met HTML");

  const char *_src = img + sizeof(img_needle) - 2;
  const char *end = strchr(_src + 1, '"');
  if (end == nullptr)
    throw std::runtime_error("Malformed IMG tag in pc_met HTML");

  const std::string_view src{_src, std::size_t(end - _src)};
  const std::string_view _name = SplitLast(src, '/').second;
  if (_name.empty())
    throw std::runtime_error("Malformed IMG tag in pc_met HTML");

  const std::string name{_name};

  // TODO: verify file name

  // TODO: delete the old directory XCSoarData/pc_met?
  const auto cache_path = MakeCacheDirectory("pc_met");
  auto path = AllocatedPath::Build(cache_path,
                                   UTF8ToWideConverter(name.c_str()));

  if (!File::Exists(path)) {
    // URI for a single page of a selected 'Satellitenbilder"-page with link
    // to the latest image and the namelist array of all stored images
    snprintf(url, sizeof(url), PCMET_URI "%.*s", int(src.size()), src.data());

    const auto ignored_response = co_await
      Net::CoDownloadToFile(curl, url, username, password,
                            path, nullptr, progress);
  }

  co_return std::move(path);
}
