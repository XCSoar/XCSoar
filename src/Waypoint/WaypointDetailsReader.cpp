// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDetailsReader.hpp"

#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Operation/ProgressListener.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "io/BufferedReader.hxx"
#include "io/ConfiguredFile.hpp"
#include "io/FileReader.hxx"
#include "io/MapFile.hpp"
#include "io/ProgressReader.hpp"
#include "io/StringConverter.hpp"
#include "io/ZipReader.hpp"
#include "system/Path.hpp"

namespace WaypointDetails {

static WaypointPtr
FindWaypoint(Waypoints &way_points, const char *name)
{
  return way_points.LookupName(name);
}

struct WaypointDetailsBuilder {
  char name[201];
  std::string details;
#ifdef HAVE_RUN_FILE
  std::forward_list<std::string> files_external;
#endif
  std::forward_list<std::string> files_embed;

  void Reset() noexcept {
    details.clear();
#ifdef HAVE_RUN_FILE
    files_external.clear();
#endif
    files_embed.clear();
  }

  void Commit(Waypoints &way_points) noexcept;
};

inline void
WaypointDetailsBuilder::Commit(Waypoints &way_points) noexcept
{
  auto wp = FindWaypoint(way_points, name);
  if (wp == nullptr)
    return;

  // TODO: eliminate this const_cast hack
  Waypoint &new_wp = const_cast<Waypoint &>(*wp);
  new_wp.details = std::move(details);

  files_embed.reverse();
  new_wp.files_embed = std::move(files_embed);

#ifdef HAVE_RUN_FILE
  files_external.reverse();
  new_wp.files_external = std::move(files_external);
#endif
}

void
ReadFile(BufferedReader &reader, Waypoints &way_points)
{
  StringConverter string_converter;
  WaypointDetailsBuilder builder;
  const char *filename;

  bool in_details = false;
  int i;

  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == '[') { // Look for start
      if (in_details)
        builder.Commit(way_points);

      builder.Reset();

      // extract name
      for (i = 1; i < 201; i++) {
        if (line[i] == ']')
          break;

        builder.name[i - 1] = line[i];
      }
      builder.name[i - 1] = 0;

      in_details = true;
    } else if ((filename =
                StringAfterPrefixIgnoreCase(line, "image=")) != nullptr) {
      builder.files_embed.emplace_front(string_converter.Convert(filename));
    } else if ((filename =
                StringAfterPrefixIgnoreCase(line, "file=")) != nullptr) {
#ifdef HAVE_RUN_FILE
      builder.files_external.emplace_front(string_converter.Convert(filename));
#endif
    } else {
      // append text to details string
      if (!StringIsEmpty(line)) {
        builder.details += string_converter.Convert(line);
        builder.details += '\n';
      }
    }
  }

  if (in_details)
    builder.Commit(way_points);
}

void
ReadFileFromProfile(Waypoints &way_points,
                    ProgressListener &progress)
{
  auto paths =
      Profile::GetMultiplePaths(ProfileKeys::AirfieldFileList, _T("*.txt\0"));
  for (const auto &path : paths) {
    try {
      auto reader = std::make_unique<FileReader>(Path(path));
      ProgressReader progress_reader{*reader, reader->GetSize(), progress};
      BufferedReader buffered_reader{progress_reader};
      ReadFile(buffered_reader, way_points);
    } catch (...) {
      LogError(std::current_exception());
    }
  }

  if (auto reader = OpenInMapFile("airfields.txt")) {
    ProgressReader progress_reader{*reader, reader->GetSize(), progress};
    BufferedReader buffered_reader{progress_reader};
    ReadFile(buffered_reader, way_points);
  }
}

} // namespace WaypointDetails
