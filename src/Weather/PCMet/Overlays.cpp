/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Overlays.hpp"
#include "Settings.hpp"
#include "Screen/Bitmap.hpp"
#include "Net/HTTP/Session.hpp"
#include "Net/HTTP/ToBuffer.hpp"
#include "Net/HTTP/ToFile.hpp"
#include "Job/Runner.hpp"
#include "LocalPath.hpp"
#include "OS/FileUtil.hpp"
#include "Util/StaticString.hxx"
#include "Util/ConvertString.hpp"
#include "Util/Macros.hpp"

#include <stdexcept>

#include <string.h>
#include <stdio.h>

#define PCMET_FTP "ftp://ftp.pcmet.de"

static constexpr const char *type_names[] = {
  "nb_cosde_ome",
};

static constexpr const TCHAR *type_labels[] = {
  _T("Vertikal"),
};

static_assert(ARRAY_SIZE(type_names) == unsigned(PCMet::OverlayInfo::Type::COUNT),
              "");

static_assert(ARRAY_SIZE(type_labels) == unsigned(PCMet::OverlayInfo::Type::COUNT),
              "");

static constexpr const char *area_names[] = {
  "nord",
  "sued",
};

static constexpr const TCHAR *area_labels[] = {
  _T("Nord"),
  _T("Süd"),
};

static_assert(ARRAY_SIZE(area_names) == unsigned(PCMet::OverlayInfo::Area::COUNT),
              "");

static_assert(ARRAY_SIZE(area_labels) == unsigned(PCMet::OverlayInfo::Area::COUNT),
              "");

static void
MakeOverlayLabel(PCMet::OverlayInfo &info)
{
  StaticString<64> label;
  label.Format(_T("%s %s %um +%uh"),
               type_labels[unsigned(info.type)],
               area_labels[unsigned(info.area)],
               info.level,
               info.step);
  info.label = label;
}

static void
FindLatestOverlay(PCMet::OverlayInfo &info)
{
  struct Visitor : public File::Visitor {
    PCMet::OverlayInfo &info;
    uint64_t latest_modification;
    uint64_t now;

    explicit Visitor(PCMet::OverlayInfo &_info)
      :info(_info), latest_modification(0), now(File::Now()) {}

    void Visit(Path path, Path) override {
      uint64_t last_modification = File::GetLastModification(path);
      if (last_modification > latest_modification &&
          last_modification <= now) {
        latest_modification = last_modification;
        info.path = path;
      }
    }
  } visitor(info);

  const auto cache_path = MakeLocalPath(_T("pc_met"));
  StaticString<256> pattern;
  pattern.Format(_T("%s_%s_lv_%06u_p_%03u_*.tiff"),
                 type_names[unsigned(info.type)],
                 area_names[unsigned(info.area)],
                 info.level, info.step);
  Directory::VisitSpecificFiles(cache_path, pattern, visitor);
}

std::list<PCMet::OverlayInfo>
PCMet::CollectOverlays()
{
  std::list<OverlayInfo> list;

  for (unsigned area = 0; area < ARRAY_SIZE(area_names); ++area) {
    for (unsigned step = 2; step <= 6; ++step) {
      OverlayInfo info;
      info.type = OverlayInfo::Type::VERTICAL;
      info.area = OverlayInfo::Area(area);
      info.level = 3000;
      info.step = step;
      MakeOverlayLabel(info);
      FindLatestOverlay(info);
      list.emplace_back(std::move(info));
    }
  }

  return list;
}

PCMet::Overlay
PCMet::DownloadOverlay(const OverlayInfo &info, BrokenDateTime now_utc,
                       const PCMetSettings &settings,
                       JobRunner &runner)
{
  const unsigned run_hour = (now_utc.hour / 3) * 3;
  unsigned run = (now_utc.hour / 3) * 300;

  NarrowString<256> url;
  url.Format(PCMET_FTP "/%s_%s_lv_%06u_p_%03u_%04u.tiff",
             type_names[unsigned(info.type)],
             area_names[unsigned(info.area)],
             info.level, info.step, run);

  const auto cache_path = MakeLocalPath(_T("pc_met"));
  auto path = AllocatedPath::Build(cache_path,
                                   UTF8ToWideConverter(url.c_str() + sizeof(PCMET_FTP)));

  {
    const WideToUTF8Converter username(settings.ftp_credentials.username);
    const WideToUTF8Converter password(settings.ftp_credentials.password);

    Net::Session session;
    Net::DownloadToFileJob job(session, url, path);
    job.SetBasicAuth(username, password);
    if (!runner.Run(job))
      return Overlay(BrokenDateTime::Invalid(),
                     BrokenDateTime::Invalid(),
                     Path(nullptr));
  }

  BrokenDateTime run_time((BrokenDate)now_utc, BrokenTime(run_hour, 0));
  if (run_hour < now_utc.hour)
    run_time.DecrementDay();

  BrokenDateTime valid_time = run_time;
  valid_time.hour += info.step;
  if (valid_time.hour >= 24) {
    valid_time.hour -= 24;
    valid_time.IncrementDay();
  }

  return Overlay(run_time, valid_time, std::move(path));
}
