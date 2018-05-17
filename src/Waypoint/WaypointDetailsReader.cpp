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

#include "WaypointDetailsReader.hpp"
#include "Language/Language.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "IO/ConfiguredFile.hpp"
#include "IO/LineReader.hpp"
#include "Operation/Operation.hpp"

#include <vector>

static WaypointPtr
FindWaypoint(Waypoints &way_points, const TCHAR *name)
{
  auto wp = way_points.LookupName(name);
  if (wp != nullptr)
    return wp;

  // TODO: Comments please! What is this supposed to do? Why do we need it?
  size_t name_length = _tcslen(name);
  TCHAR buffer[name_length + 4];
  _tcscpy(buffer, name);
  _tcscpy(buffer + name_length, _T(" AF"));
  wp = way_points.LookupName(buffer);
  if (wp != nullptr)
    return wp;

  _tcscpy(buffer + name_length, _T(" AD"));
  wp = way_points.LookupName(buffer);
  if (wp != nullptr)
    return wp;

  return nullptr;
}

static void
SetAirfieldDetails(Waypoints &way_points, const TCHAR *name,
                   const tstring &Details,
                   const std::vector<tstring> &files_external,
                   const std::vector<tstring> &files_embed)
{
  auto wp = FindWaypoint(way_points, name);
  if (wp == nullptr)
    return;

  // TODO: eliminate this const_cast hack
  Waypoint &new_wp = const_cast<Waypoint &>(*wp);
  new_wp.details = Details.c_str();
  new_wp.files_embed.assign(files_embed.begin(), files_embed.end());
#ifdef HAVE_RUN_FILE
  new_wp.files_external.assign(files_external.begin(), files_external.end());
#endif
}

/**
 * Parses the data provided by the airfield details file handle
 */
static void
ParseAirfieldDetails(Waypoints &way_points, TLineReader &reader,
                     OperationEnvironment &operation)
{
  tstring details;
  std::vector<tstring> files_external, files_embed;
  TCHAR name[201];
  const TCHAR *filename;

  name[0] = 0;

  bool in_details = false;
  int i;

  const long filesize = std::max(reader.GetSize(), 1l);
  operation.SetProgressRange(100);

  TCHAR *line;
  while ((line = reader.ReadLine()) != nullptr) {
    if (line[0] == _T('[')) { // Look for start
      if (in_details)
        SetAirfieldDetails(way_points, name, details, files_external,
                           files_embed);

      details.clear();
      files_external.clear();
      files_embed.clear();

      // extract name
      for (i = 1; i < 201; i++) {
        if (line[i] == _T(']'))
          break;

        name[i - 1] = line[i];
      }
      name[i - 1] = 0;

      in_details = true;

      operation.SetProgressPosition(reader.Tell() * 100 / filesize);
    } else if ((filename =
                StringAfterPrefixCI(line, _T("image="))) != nullptr) {
      files_embed.emplace_back(filename);
    } else if ((filename =
                StringAfterPrefixCI(line, _T("file="))) != nullptr) {
#ifdef HAVE_RUN_FILE
      files_external.emplace_back(filename);
#endif
    } else {
      // append text to details string
      if (!StringIsEmpty(line)) {
        details += line;
        details += _T('\n');
      }
    }
  }

  if (in_details)
    SetAirfieldDetails(way_points, name, details, files_external, files_embed);
}

/**
 * Opens the airfield details file and parses it
 */
void
WaypointDetails::ReadFile(TLineReader &reader, Waypoints &way_points,
                          OperationEnvironment &operation)
{
  operation.SetText(_("Loading Airfield Details File..."));
  ParseAirfieldDetails(way_points, reader, operation);
}

void
WaypointDetails::ReadFileFromProfile(Waypoints &way_points,
                                     OperationEnvironment &operation)
{
  auto reader = OpenConfiguredTextFile(ProfileKeys::AirfieldFile,
                                       "airfields.txt",
                                       Charset::AUTO);
  if (reader)
    ReadFile(*reader, way_points, operation);
}
