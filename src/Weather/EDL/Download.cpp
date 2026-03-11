// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Download.hpp"
#include "Request.hpp"
#include "net/http/CoDownloadToFile.hpp"
#include "system/FileUtil.hpp"

namespace EDL {

Co::Task<AllocatedPath>
EnsureDownloaded(BrokenDateTime forecast, unsigned isobar,
                 CurlGlobal &curl, ProgressListener &progress)
{
  auto path = BuildCachePath(forecast, isobar);
  if (!File::ExistsAny(path)) {
    const auto url = BuildDownloadUrl(forecast, isobar);
    const auto ignored = co_await Net::CoDownloadToFile(curl, url.c_str(),
                                                        nullptr, nullptr,
                                                        path, nullptr,
                                                        progress);
    (void)ignored;
  }

  co_return path;
}

} // namespace EDL
