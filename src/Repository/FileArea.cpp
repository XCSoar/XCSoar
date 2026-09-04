// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileArea.hpp"
#include "util/CharUtil.hxx"

#include <algorithm>

std::string
NormalizeFileArea(std::string_view area)
{
  std::string out;
  out.reserve(area.size());
  for (const char ch : area) {
    if (ch == '\0')
      break;
    out.push_back(ToUpperASCII(ch));
  }
  return out;
}

bool
FileMatchesArea(const AvailableFile &file, std::string_view area)
{
  return NormalizeFileArea(file) == area;
}

std::vector<std::string>
CollectUniqueFileAreas(const std::vector<AvailableFile> &files)
{
  std::vector<std::string> areas;
  areas.reserve(files.size());
  for (const auto &file : files)
    areas.push_back(NormalizeFileArea(file));

  std::sort(areas.begin(), areas.end());
  areas.erase(std::unique(areas.begin(), areas.end()), areas.end());
  return areas;
}

unsigned
CountFilesInArea(const std::vector<AvailableFile> &files,
                 std::string_view area)
{
  unsigned count = 0;
  for (const auto &file : files)
    if (FileMatchesArea(file, area))
      ++count;
  return count;
}

void
AppendFilesInArea(const std::vector<AvailableFile> &files,
                  std::string_view area,
                  std::vector<AvailableFile> &dest)
{
  for (const auto &file : files)
    if (FileMatchesArea(file, area))
      dest.push_back(file);
}
