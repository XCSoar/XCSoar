// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Satellite.hpp"
#include "net/http/CoDownloadToFile.hpp"
#include "co/Task.hxx"
#include "io/FileMapping.hpp"
#include "time/BrokenDateTime.hpp"
#include "LocalPath.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <span>
#include <stdexcept>
#include <string_view>

Co::Task<AllocatedPath>
EUMETView::DownloadTile(const Layer &layer, const GeoBitmap::TileData &tile,
                        const BrokenDateTime &frame_time,
                        CurlGlobal &curl, ProgressListener &progress)
{
  const auto url = MakeTileURL(layer, tile, frame_time);
  if (url.empty())
    throw std::runtime_error("No tile to fetch");

  const auto cache = MakeCacheDirectory("eumetview");

  /* one file per tile of the current frame, so that a tile already on
     disk is not fetched again while the aircraft moves across it, and
     the name follows the "satellite-<zoom>-<x>-<y>" convention the
     rest of XCSoar uses for georeferenced tiles */
  auto path = AllocatedPath::Build(cache,
                                   fmt::format("satellite-{}-{}-{}.png",
                                               tile.zoom, tile.x,
                                               tile.y).c_str());

  {
    const auto ignored_response = co_await
      Net::CoDownloadToFile(curl, url.c_str(), nullptr, nullptr,
                            path, nullptr, progress);
  }

  /* EUMETView reports its errors as a WMS ServiceExceptionReport
     carried in an ordinary "200 OK", so the file we just wrote may
     well be XML.  Catch that here, where the message can still be
     read, rather than letting it fail namelessly in the bitmap
     loader. */
  {
    const FileMapping mapping{path};
    const std::span<const std::byte> data{mapping};

    if (!IsPNG(data)) {
      const std::string_view text{(const char *)data.data(),
                                  std::min(data.size(), std::size_t(4096))};

      if (auto message = ExtractServiceException(text); !message.empty())
        throw std::runtime_error(std::move(message));

      throw std::runtime_error("The server did not return an image");
    }
  }

  co_return std::move(path);
}
