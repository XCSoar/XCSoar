/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Images.hpp"
#include "Settings.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "net/http/ToBuffer.hpp"
#include "net/http/ToFile.hpp"
#include "Job/Runner.hpp"
#include "LocalPath.hpp"
#include "system/FileUtil.hpp"
#include "util/ConvertString.hpp"

#include <stdexcept>

#include <string.h>
#include <stdio.h>

#define PCMET_URI "https://www.flugwetter.de"

static constexpr PCMet::ImageArea rad_lokal_areas[] = {
  { "pro", _T("Prötzel") },
  { "drs", _T("Dresden") },
  { "eis", _T("Eisberg") },
  { "emd", _T("Emden") },
  { "ess", _T("Essen") },
  { "fbg", _T("Feldberg") },
  { "fld", _T("Flechtdorf") },
  { "boo", _T("Boostedt") },
  { "hnr", _T("Hannover") },
  { "mem", _T("Memmingen") },
  { "isn", _T("Isen") },
  { "neu", _T("Neuhaus") },
  { "nhb", _T("Neuheilenbach") },
  { "oft", _T("Offenthal") },
  { "ros", _T("Rostock") },
  { "tur", _T("Türkheim") },
  { "umd", _T("Ummendorf") },
  { "eddb", _T("EDDB") },
  { "eddc", _T("EDDC") },
  { "edde", _T("EDDE") },
  { "eddf", _T("EDDF") },
  { "eddg", _T("EDDG") },
  { "eddh", _T("EDDH") },
  { "eddk", _T("EDDK") },
  { "eddl", _T("EDDL") },
  { "eddm", _T("EDDM") },
  { "eddn", _T("EDDN") },
  { "eddp", _T("EDDP") },
  { "eddr", _T("EDDR") },
  { "edds", _T("EDDS") },
  { "eddt", _T("EDDT") },
  { "eddv", _T("EDDV") },
  { "eddw", _T("EDDW") },
  { nullptr, nullptr }
};

static constexpr PCMet::ImageArea rad_areas[] = {
  { "de", _T("Deutschland") },
  { "eu", _T("Europa") },
  { nullptr, nullptr }
};

static constexpr PCMet::ImageArea sat_areas[] = {
  { "vis_hrv_eu", _T("Mitteleuropa HRV") },
  { "ir_rgb_eu", _T("Mitteleuropa RGB") },
  { "ir_108_eu", _T("Mitteleuropa IR") },
  { "vis_hrv_ce", _T("Mitteleuropa HRV") },
  { "ir_rgb_ce", _T("Mitteleuropa RGB") },
  { "ir_108_ce", _T("Mitteleuropa IR") },
  { "vis_hrv_mdl", _T("Deutschland HRV") },
  { "ir_rgb_mdl", _T("Deutschland RGB") },
  { "ir_108_mdl", _T("Deutschland IR") },
  { "vis_hrv_ndl", _T("Deutschland Nord HRV") },
  { "ir_rgb_ndl", _T("Deutschland Nord RGB") },
  { "ir_108_ndl", _T("Deutschland Nord IR") },
  { "vis_hrv_sdl", _T("Deutschland Süd HRV") },
  { "ir_rgb_sdl", _T("Deutschland Süd RGB") },
  { "ir_108_sdl", _T("Deutschland Süd IR") },
  { nullptr, nullptr }
};

const PCMet::ImageType PCMet::image_types[] = {
  { "rad_lokal/einzelstandorte.htm", _T("Lokale RADAR-Bilder"), rad_lokal_areas },
  { "rad/index.htm", _T("RADAR"), rad_areas },
  { "sat/index.htm", _T("Satellitenbilder"), sat_areas },
  { nullptr, nullptr, nullptr },
};

Bitmap
PCMet::DownloadLatestImage(const char *type, const char *area,
                           const PCMetSettings &settings,
                           CurlGlobal &curl, JobRunner &runner)
{
  const WideToUTF8Converter username(settings.www_credentials.username);
  const WideToUTF8Converter password(settings.www_credentials.password);

  char url[256];
  snprintf(url, sizeof(url),
           PCMET_URI "/fw/bilder/%s?type=%s",
           type, area);

  // download the HTML page
  char buffer[65536];
  Net::DownloadToBufferJob job(curl, url, buffer, sizeof(buffer) - 1);
  job.SetBasicAuth(username, password);
  if (!runner.Run(job))
    return Bitmap();

  buffer[job.GetLength()] = 0;

  static constexpr char img_needle[] = "<img name=\"bild\" src=\"/";
  char *img = strstr(buffer, "<img name=\"bild\" src=\"/");
  if (img == nullptr)
    return Bitmap();

  char *src = img + sizeof(img_needle) - 2;
  char *end = strchr(src + 1, '"');
  if (end == nullptr)
    return Bitmap();

  *end = 0;

  const char *slash = strrchr(src, '/');
  if (slash == nullptr || slash[1] == 0)
    return Bitmap();

  const char *name = slash + 1;

  // TODO: verify file name

  const auto cache_path = MakeLocalPath(_T("pc_met"));
  const auto path = AllocatedPath::Build(cache_path,
                                         UTF8ToWideConverter(name));

  if (!File::Exists(path)) {
    // URI for a single page of a selected 'Satellitenbilder"-page with link
    // to the latest image and the namelist array of all stored images
    snprintf(url, sizeof(url), PCMET_URI "%s", src);

    Net::DownloadToFileJob job2(curl, url, path);
    job2.SetBasicAuth(username, password);
    if (!runner.Run(job2))
      return Bitmap();
  }

  Bitmap bitmap;
  try {
    bitmap.LoadFile(path);
  } catch (const std::runtime_error &e) {
  }

  return bitmap;
}
