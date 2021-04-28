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

#include "Parser.hpp"
#include "FileRepository.hpp"
#include "io/LineReader.hpp"
#include "util/StringStrip.hxx"
#include "util/HexString.hpp"

/**
 * Parses a line of the repository file.
 * Each line is of the form `name = value`
 * @returns A pointer to the value field.
 */
static const char *
ParseLine(char *line)
{
  char *separator = strchr(line, '=');
  if (separator == nullptr)
    /* malformed line */
    return nullptr;

  char *p = StripRight(line, separator);
  if (p == line)
    /* empty name */
    return nullptr;

  *p = 0;

  char *value = const_cast<char *>(StripLeft(separator + 1));
  StripRight(value);
  return value;
}

static bool
Commit(FileRepository &repository, AvailableFile &file)
{
  if (file.IsEmpty())
    return true;

  if (!file.IsValid())
    return false;

  repository.files.emplace_back(std::move(file));
  file.Clear();
  return true;
}

bool
ParseFileRepository(FileRepository &repository, NLineReader &reader)
{
  AvailableFile file;
  file.Clear();

  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    line = const_cast<char *>(StripLeft(line));
    if (*line == 0 || *line == '#')
      continue;

    const char *name = line, *value = ParseLine(line);
    if (value == nullptr)
      return false;

    if (StringIsEqual(name, "name")) {
      if (!Commit(repository, file))
        return false;

      file.name.assign(value);
    } else if (file.IsEmpty()) {
      /* ignore */
    } else if (StringIsEqual(name, "uri")) {
      file.uri.assign(value);
    } else if (StringIsEqual(name, "area")) {
      file.area = value;
    } else if (StringIsEqual(name, "type")) {
      if (StringIsEqual(value, "airspace"))
        file.type = FileType::AIRSPACE;
      else if (StringIsEqual(value, "waypoint-details"))
        file.type = FileType::WAYPOINTDETAILS;
      else if (StringIsEqual(value, "waypoint"))
        file.type = FileType::WAYPOINT;
      else if (StringIsEqual(value, "map"))
        file.type = FileType::MAP;
      else if (StringIsEqual(value, "flarmnet"))
        file.type = FileType::FLARMNET;
    } else if (StringIsEqual(name, "update")) {
      int year, month, day;
      if (sscanf(value, "%04u-%02u-%02u", &year, &month, &day) == 3)
        file.update_date = BrokenDate(year, month, day);
    } else if (StringIsEqual(name, "sha256")) {
      try {
        file.sha256_hash = ParseHexString<32>(std::string_view(value));
      } catch (std::exception &e) {
        // Parsing failed, sha256_hash stays zeroed
      }
    }
  }

  return Commit(repository, file);
}
