// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDetailsReader.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "io/ConfiguredFile.hpp"
#include "io/LineReader.hpp"
#include "Operation/ProgressListener.hpp"

namespace WaypointDetails {

static WaypointPtr
FindWaypoint(Waypoints &way_points, const TCHAR *name)
{
  return way_points.LookupName(name);
}

struct WaypointDetailsBuilder {
  TCHAR name[201];
  tstring details;
#ifdef HAVE_RUN_FILE
  std::forward_list<tstring> files_external;
#endif
  std::forward_list<tstring> files_embed;

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

/**
 * Parses the data provided by the airfield details file handle
 */
static void
ParseAirfieldDetails(Waypoints &way_points, TLineReader &reader,
                     ProgressListener &progress)
{
  WaypointDetailsBuilder builder;
  const TCHAR *filename;

  bool in_details = false;
  int i;

  const long filesize = std::max(reader.GetSize(), 1l);
  progress.SetProgressRange(100);

  TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == _T('[')) { // Look for start
      if (in_details)
        builder.Commit(way_points);

      builder.Reset();

      // extract name
      for (i = 1; i < 201; i++) {
        if (line[i] == _T(']'))
          break;

        builder.name[i - 1] = line[i];
      }
      builder.name[i - 1] = 0;

      in_details = true;

      progress.SetProgressPosition(reader.Tell() * 100 / filesize);
    } else if ((filename =
                StringAfterPrefixIgnoreCase(line, _T("image="))) != nullptr) {
      builder.files_embed.emplace_front(filename);
    } else if ((filename =
                StringAfterPrefixIgnoreCase(line, _T("file="))) != nullptr) {
#ifdef HAVE_RUN_FILE
      builder.files_external.emplace_front(filename);
#endif
    } else {
      // append text to details string
      if (!StringIsEmpty(line)) {
        builder.details += line;
        builder.details += _T('\n');
      }
    }
  }

  if (in_details)
    builder.Commit(way_points);
}

/**
 * Opens the airfield details file and parses it
 */
void
ReadFile(TLineReader &reader, Waypoints &way_points,
         ProgressListener &progress)
{
  ParseAirfieldDetails(way_points, reader, progress);
}

void
ReadFileFromProfile(Waypoints &way_points,
                    ProgressListener &progress)
{
  auto reader = OpenConfiguredTextFile(ProfileKeys::AirfieldFile,
                                       "airfields.txt",
                                       Charset::AUTO);
  if (reader)
    ReadFile(*reader, way_points, progress);
}

} // namespace WaypointDetails
