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
#include "util/StringAPI.hxx"
#include "util/StringSplit.hxx"

#include <stdexcept>

#include <stdio.h>
#include <string>

#define PCMET_URI "https://www.flugwetter.de"

static constexpr PCMet::ImageArea rad_lokal_areas[] = {
  { "pro", "Prötzel" },
  { "drs", "Dresden" },
  { "eis", "Eisberg" },
  { "asb", "Borkum" },
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
  { "rx", "Deutschland" },
  { "rxn", "Deutschland Nord" },
  { "rxm", "Deutschland Mitte" },
  { "rxs", "Deutschland Süd" },
  { "eu", "Europa" },
  { "fa", "Alpen" },
  { nullptr, nullptr }
};

static constexpr PCMet::ImageArea satradblitz_areas[] = {
  { "eh", "Europa" },
  { nullptr, nullptr }
};

static constexpr PCMet::ImageArea blitzkarte_areas[] = {
  { "0", "Europa" },
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
  { "satradblitz/index.htm", "SAT RAD BLITZ", satradblitz_areas },
  { "blitzkarte_bild.htm", "Blitzkarte", blitzkarte_areas },
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

[[gnu::pure]]
static bool
IsBlitzkartePage(const char *type) noexcept
{
  return StringIsEqual(type, "blitzkarte_bild.htm");
}

/**
 * Resolve a pc_met image src attribute to an absolute path (from site root).
 */
[[gnu::pure]]
static std::string
ResolveImageSrc(std::string_view src) noexcept
{
  if (!src.empty() && src.front() == '/')
    return std::string(src);

  if (src.starts_with("../scripts/"))
    return std::string("/fw/") + std::string(src.substr(3));

  return std::string(src);
}

/**
 * Parse the main image URL from a pc_met HTML page.
 */
[[gnu::pure]]
static std::string_view
FindImageSrc(const char *html) noexcept
{
  static constexpr char bild_needle[] = "<img name=\"bild\" src=\"/";
  if (const char *img = StringFind(html, bild_needle)) {
    const char *src = img + sizeof(bild_needle) - 2;
    const char *end = StringFind(src + 1, '"');
    if (end != nullptr)
      return {src, std::size_t(end - src)};
  }

  static constexpr char karte_needle[] = "src=\"../scripts/karte.php?";
  if (const char *img = StringFind(html, karte_needle)) {
    const char *src = img + sizeof("src=\"") - 1;
    const char *end = StringFind(src, '"');
    if (end != nullptr)
      return {src, std::size_t(end - src)};
  }

  return {};
}

/**
 * Derive a safe local cache file name from an image URL path.
 */
[[gnu::pure]]
static std::string
CacheFileNameFromImageSrc(std::string_view src) noexcept
{
  static constexpr char src_param[] = "src=";
  if (const auto pos = src.find(src_param);
      pos != std::string_view::npos)
    src = src.substr(pos + sizeof(src_param) - 1);

  if (const auto q = src.find_first_of("?#");
      q != std::string_view::npos)
    src = src.substr(0, q);

  const std::string_view name = SplitLast(src, '/').second;
  if (name.empty())
    return {};

  if (name == "karte.php")
    return "blitzkarte.png";

  return std::string(name);
}

Co::Task<AllocatedPath>
PCMet::DownloadLatestImage(const char *type, const char *area,
                           const PCMetSettings &settings,
                           CurlGlobal &curl, ProgressListener &progress)
{
  char url[256];
  if (IsBlitzkartePage(type))
    snprintf(url, sizeof(url),
             PCMET_URI "/fw/bilder/%s?maxage=%s",
             type, area);
  else
    snprintf(url, sizeof(url),
             PCMET_URI "/fw/bilder/%s?type=%s",
             type, area);

  // download the HTML page
  const auto response =
    co_await CoGet(curl, url, settings.www_credentials.username,
                   settings.www_credentials.password, progress);

  const std::string_view src_view = FindImageSrc(response.body.c_str());
  if (src_view.empty())
    throw std::runtime_error("No IMG tag found in pc_met HTML");

  const std::string src = ResolveImageSrc(src_view);
  const std::string name = CacheFileNameFromImageSrc(src);
  if (name.empty())
    throw std::runtime_error("Malformed IMG tag in pc_met HTML");

  // TODO: verify file name

  // TODO: delete the old directory XCSoarData/pc_met?
  const auto cache_path = MakeCacheDirectory("pc_met");
  auto path = AllocatedPath::Build(cache_path, name.c_str());

  if (!File::Exists(path)) {
    snprintf(url, sizeof(url), PCMET_URI "%s", src.c_str());

    const auto ignored_response = co_await
      Net::CoDownloadToFile(curl, url, settings.www_credentials.username,
                            settings.www_credentials.password,
                            path, nullptr, progress);
  }

  co_return std::move(path);
}
