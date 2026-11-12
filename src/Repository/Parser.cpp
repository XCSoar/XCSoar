// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Parser.hpp"
#include "FileRepository.hpp"
#include "io/LineReader.hpp"
#include "util/StringStrip.hxx"
#include "util/HexString.hpp"
#include "system/Path.hpp"

#include <array>
#include <string_view>

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

  const Path path(file.name.c_str());
  if (!path.IsValidFilename())
    return false;

  if (!file.IsValid())
    return false;

  repository.files.emplace_back(std::move(file));
  file.Clear();
  return true;
}

struct FileTypeMapping {
  std::string_view name;
  FileType type;
};

static constexpr std::array FILE_TYPE_MAPPINGS{
  FileTypeMapping{"airspace", FileType::AIRSPACE},
  FileTypeMapping{"waypoint-details", FileType::WAYPOINTDETAILS},
  FileTypeMapping{"waypoint", FileType::WAYPOINT},
  FileTypeMapping{"map", FileType::MAP},
  FileTypeMapping{"flarmnet", FileType::FLARMNET},
  FileTypeMapping{"rasp", FileType::RASP},
  FileTypeMapping{"xci", FileType::XCI},
  FileTypeMapping{"task", FileType::TASK},
  FileTypeMapping{"checklist", FileType::CHECKLIST},
  FileTypeMapping{"software-update", FileType::SOFTWARE_UPDATE},
};

static FileType
ParseFileType(std::string_view value) noexcept
{
  for (const auto &mapping : FILE_TYPE_MAPPINGS)
    if (value == mapping.name)
      return mapping.type;

  return FileType::UNKNOWN;
}

struct SoftwareUpdateFieldMapping {
  std::string_view name;
  std::string SoftwareUpdateMetadata::*field;
};

static constexpr std::array SOFTWARE_UPDATE_FIELD_MAPPINGS{
  SoftwareUpdateFieldMapping{"target", &SoftwareUpdateMetadata::target},
  SoftwareUpdateFieldMapping{"version", &SoftwareUpdateMetadata::version},
  SoftwareUpdateFieldMapping{"channel", &SoftwareUpdateMetadata::channel},
  SoftwareUpdateFieldMapping{"offer-id", &SoftwareUpdateMetadata::offer_id},
  SoftwareUpdateFieldMapping{"source", &SoftwareUpdateMetadata::source},
};

static bool
AssignSoftwareUpdateField(AvailableFile &file, std::string_view name,
                          const char *value)
{
  for (const auto &mapping : SOFTWARE_UPDATE_FIELD_MAPPINGS)
    if (name == mapping.name) {
      if (!file.software_update)
        file.software_update.emplace();
      (*file.software_update).*mapping.field = value;
      return true;
    }

  return false;
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
    } else if (StringIsEqual(name, "description")) {
      file.description.assign(value);
    } else if (AssignSoftwareUpdateField(file, name, value)) {
      /* assigned above */
    } else if (StringIsEqual(name, "area")) {
      file.area = value;
    } else if (StringIsEqual(name, "type")) {
      file.type = ParseFileType(value);
      if (file.type == FileType::SOFTWARE_UPDATE && !file.software_update)
        file.software_update.emplace();
    } else if (StringIsEqual(name, "update")) {
      unsigned year, month, day;
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
