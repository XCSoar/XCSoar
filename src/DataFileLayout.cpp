// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DataFileLayout.hpp"

#include "LocalPath.hpp"
#include "Repository/FileType.hpp"
#include "Waypoint/WaypointDetailsFormat.hpp"
#include "io/BufferedReader.hxx"
#include "io/FileReader.hxx"
#include "system/FileUtil.hpp"
#include "util/StringCompare.hxx"
#include "util/StringStrip.hxx"

#include <string_view>

namespace {

enum class SniffedAirspaceLine {
  UNKNOWN,
  OPENAIR,
  TNP,
};

[[gnu::pure]]
SniffedAirspaceLine
SniffAirspaceLine(std::string_view line) noexcept
{
  if (StringStartsWithIgnoreCase(line, "INCLUDE=") ||
      StringStartsWithIgnoreCase(line, "TYPE=") ||
      StringStartsWithIgnoreCase(line, "TITLE="))
    return SniffedAirspaceLine::TNP;

  if (StringStartsWithIgnoreCase(line, "AC")) {
    const auto rest = line.substr(2);
    if (rest.size() >= 2 && rest.front() == ' ' &&
        rest.find_first_not_of(' ', 1) != std::string_view::npos)
      return SniffedAirspaceLine::OPENAIR;
  }

  return SniffedAirspaceLine::UNKNOWN;
}

FileType
SniffAmbiguousTextFileType(Path path) noexcept
{
  try {
    FileReader reader(path);
    BufferedReader buffered_reader(reader);

    unsigned airspace_lines = 0;
    bool details_section = false;
    bool details_attachment = false;
    unsigned inspected_lines = 0;

    char *line;
    while ((line = buffered_reader.ReadLine()) != nullptr &&
           inspected_lines < 64) {
      const std::string_view trimmed = Strip(std::string_view{line});
      if (trimmed.empty())
        continue;

      ++inspected_lines;

      if (WaypointDetails::IsSectionHeader(trimmed)) {
        details_section = true;
        continue;
      }

      if (WaypointDetails::IsAttachmentLine(trimmed)) {
        if (details_section)
          details_attachment = true;
        continue;
      }

      const auto airspace_type = SniffAirspaceLine(trimmed);
      if (airspace_type != SniffedAirspaceLine::UNKNOWN) {
        ++airspace_lines;
        if (!details_section && !details_attachment &&
            (airspace_type == SniffedAirspaceLine::OPENAIR ||
             airspace_lines >= 2))
          return FileType::AIRSPACE;
        continue;
      }

      if (airspace_lines > 0 && details_section)
        return FileType::UNKNOWN;
    }

    if (details_section && details_attachment && airspace_lines == 0)
      return FileType::WAYPOINTDETAILS;
  } catch (...) {
  }

  return FileType::UNKNOWN;
}

} // namespace

AllocatedPath
DataFileLayout::GetLayoutSubdirForDataFile(Path path) noexcept
{
  if (path == nullptr)
    return nullptr;

  const auto base = path.GetBase();
  if (base == nullptr)
    return nullptr;

  if (!Path(base).EndsWithIgnoreCase(".txt"))
    return GetLayoutSubdirForFilename(base.c_str());

  const auto special = SpecialFilenameType(base.c_str());
  if (special != FileType::UNKNOWN)
    return GetFileTypeDefaultDir(special);

  const auto sniffed = SniffAmbiguousTextFileType(path);
  return sniffed != FileType::UNKNOWN
    ? GetFileTypeDefaultDir(sniffed)
    : nullptr;
}

AllocatedPath
DataFileLayout::RelocateRootDataFileToLayoutSubdir(Path path) noexcept
{
  if (path == nullptr)
    return nullptr;

  const auto relative = RelativePath(path);
  if (relative == nullptr || !relative.IsBase())
    return AllocatedPath(path);

  const auto base = relative.GetBase();
  if (base == nullptr)
    return AllocatedPath(path);

  auto subdir = GetLayoutSubdirForDataFile(path);
  if (subdir == nullptr)
    return AllocatedPath(path);

  auto destination = LocalPath(AllocatedPath::Build(subdir, Path(base)));
  if (destination == nullptr || destination == path || File::ExistsAny(destination))
    return AllocatedPath(path);

  if (const auto parent = destination.GetParent(); parent != nullptr)
    Directory::CreateRecursive(parent);

  if (File::Rename(path, destination))
    return destination;

  return AllocatedPath(path);
}
