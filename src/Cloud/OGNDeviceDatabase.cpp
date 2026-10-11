// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OGNDeviceDatabase.hpp"
#include "io/BufferedReader.hxx"
#include "io/FileReader.hxx"
#include "system/Path.hpp"
#include "util/IterableSplitString.hxx"
#include "util/NumberParser.hxx"

#include <array>
#include <iostream>

using std::string_view_literals::operator""sv;

static constexpr std::string_view HEADER =
  "#DEVICE_TYPE,DEVICE_ID,AIRCRAFT_MODEL,REGISTRATION,CN,TRACKED,IDENTIFIED"sv;

/**
 * Split one CSV line of the database.  Fields are quoted with
 * single quotes and may contain commas.
 *
 * @return false if the line does not have the seven fields
 */
static bool
SplitLine(std::string_view line,
          std::array<std::string_view, 7> &fields) noexcept
{
  std::size_t n = 0;

  while (!line.empty()) {
    if (n == fields.size())
      return false;

    if (line.front() == '\'') {
      const auto end = line.find('\'', 1);
      if (end == line.npos)
        return false;

      fields[n++] = line.substr(1, end - 1);
      line.remove_prefix(end + 1);
    } else {
      const auto end = line.find(',');
      fields[n++] = line.substr(0, end);
      line.remove_prefix(end == line.npos ? line.size() : end);
    }

    if (!line.empty()) {
      if (line.front() != ',')
        return false;
      line.remove_prefix(1);
    }
  }

  return n == fields.size();
}

/**
 * The OGN address type of a database device type: "F" FLARM, "I"
 * ICAO, "O" OGN tracker.
 */
static unsigned
ToAddressType(std::string_view device_type) noexcept
{
  if (device_type == "F"sv)
    return 2;
  if (device_type == "I"sv)
    return 1;
  if (device_type == "O"sv)
    return 3;
  return 0;
}

/**
 * Add the device of one line to the set if its owner does not want
 * it tracked or identified.
 *
 * @return false if the line is malformed, e.g. cut off by an
 * incomplete download
 */
static bool
ParseLine(std::string_view line,
          std::unordered_set<uint32_t> &blocked) noexcept
{
  if (!line.empty() && line.back() == '\r')
    line.remove_suffix(1);

  if (line.empty())
    return true;

  std::array<std::string_view, 7> fields;
  if (!SplitLine(line, fields))
    return false;

  const unsigned type = ToAddressType(fields[0]);
  const auto address = ParseInteger<uint32_t>(fields[1], 16);
  if (type == 0 || !address || *address > 0xffffff)
    return true;

  /* anything but an explicit "Y" counts as a "no" */
  if (fields[5] != "Y"sv || fields[6] != "Y"sv)
    blocked.insert((type << 24) | *address);

  return true;
}

bool
OGNDeviceDatabase::Parse(std::string_view csv) noexcept
{
  std::unordered_set<uint32_t> new_blocked;
  bool header = false;

  for (const auto line : IterableSplitString(csv, '\n')) {
    if (!header) {
      if (line.starts_with(HEADER))
        header = true;
      continue;
    }

    /* a malformed line may have dropped a blocked device: keep the
       old database */
    if (!ParseLine(line, new_blocked))
      return false;
  }

  if (!header)
    return false;

  blocked = std::move(new_blocked);
  loaded = true;
  return true;
}

bool
OGNDeviceDatabase::LoadFile(Path path) noexcept
try {
  FileReader file(path);
  BufferedReader reader(file);

  const char *first = reader.ReadLine();
  if (first == nullptr || !std::string_view{first}.starts_with(HEADER))
    return false;

  std::unordered_set<uint32_t> new_blocked;
  while (const char *line = reader.ReadLine())
    if (!ParseLine(line, new_blocked)) {
      std::cerr << "OGN\tddb-malformed\t" << path.c_str() << std::endl;
      return false;
    }

  blocked = std::move(new_blocked);
  loaded = true;
  return true;
} catch (...) {
  std::cerr << "OGN\tddb-error\t" << path.c_str() << std::endl;
  return false;
}
