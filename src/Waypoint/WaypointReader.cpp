// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointReader.hpp"
#include "WaypointReaderZander.hpp"
#include "WaypointReaderSeeYou.hpp"
#include "WaypointReaderWinPilot.hpp"
#include "WaypointReaderFS.hpp"
#include "WaypointReaderOzi.hpp"
#include "WaypointReaderCompeGPS.hpp"
#include "WaypointFileType.hpp"
#include "system/Path.hpp"
#include "io/FileReader.hxx"
#include "io/CupxArchive.hpp"
#include "io/MemoryReader.hxx"
#include "io/ZipReader.hpp"
#include "io/ProgressReader.hpp"
#include "io/BufferedReader.hxx"

#include "util/Compiler.h"

#include <memory>

static WaypointReaderBase *
CreateWaypointReader(WaypointFileType type, WaypointFactory factory)
{
  switch (type) {
  case WaypointFileType::UNKNOWN:
    break;

  case WaypointFileType::WINPILOT:
    return new WaypointReaderWinPilot(factory);

  case WaypointFileType::SEEYOU:
    break;

  case WaypointFileType::CUPX:
    gcc_unreachable();

  case WaypointFileType::ZANDER:
    return new WaypointReaderZander(factory);

  case WaypointFileType::FS:
    return new WaypointReaderFS(factory);

  case WaypointFileType::OZI_EXPLORER:
    return new WaypointReaderOzi(factory);

  case WaypointFileType::COMPE_GPS:
    return new WaypointReaderCompeGPS(factory);
  }

  return nullptr;
}

static void
ReadWaypointFile(Reader &file_reader, WaypointFileType file_type,
                 uint_least64_t total_size,
                 Waypoints &way_points, WaypointFactory factory,
                 ProgressListener &progress)
{
  ProgressReader progress_reader{file_reader, total_size, progress};
  BufferedReader buffered_reader{progress_reader};

  switch (file_type) {
  case WaypointFileType::SEEYOU:
    ParseSeeYou(factory, way_points, buffered_reader);
    break;

  case WaypointFileType::CUPX:
    gcc_unreachable();
  default:
    std::unique_ptr<WaypointReaderBase> reader { CreateWaypointReader(file_type,
                                                                      factory) };
    if (!reader)
      throw std::runtime_error{"Unrecognised waypoint file"};

    reader->Parse(way_points, buffered_reader);
    break;
  }
}

void
ReadWaypointFile(Path path, WaypointFileType file_type,
                 Waypoints &way_points,
                 WaypointFactory factory, ProgressListener &progress)
{
  if (file_type == WaypointFileType::CUPX) {
    auto cup_data = CupxArchive::ExtractPointsCup(path);
    if (cup_data.empty())
      throw std::runtime_error{"Failed to read POINTS.CUP from CUPX archive"};

    MemoryReader mem_reader{cup_data};
    ProgressReader progress_reader{mem_reader, cup_data.size(), progress};
    BufferedReader buffered_reader{progress_reader};
    ParseSeeYou(factory, way_points, buffered_reader);
    return;
  }

  FileReader file_reader{path};
  ReadWaypointFile(file_reader, file_type, file_reader.GetSize(),
                   way_points, factory, progress);
}

void
ReadWaypointFile(Path path, Waypoints &way_points,
                 WaypointFactory factory, ProgressListener &progress)
{
  ReadWaypointFile(path, DetermineWaypointFileType(path),
                   way_points, factory, progress);
}

void
ReadWaypointFile(struct zzip_dir *dir, const char *path,
                 WaypointFileType file_type, Waypoints &way_points,
                 WaypointFactory factory, ProgressListener &progress)
{
  ZipReader file_reader{dir, path};
  ReadWaypointFile(file_reader, file_type, file_reader.GetSize(),
                   way_points, factory, progress);
}
