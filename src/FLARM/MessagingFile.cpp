// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MessagingFile.hpp"
#include "MessagingDatabase.hpp"
#include "MessagingRecord.hpp"
#include "thread/SharedMutex.hpp"
#include "io/BufferedReader.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/StringConverter.hpp"
#include "util/ConvertString.hpp"
#include "io/BufferedCsvReader.hpp"
#include "util/StringStrip.hxx"
#include "util/HexString.hpp"

#include <string_view>

/**
 * Write a CSV field with proper escaping according to RFC 4180.
 * Fields are quoted if they contain comma, newline, or quote characters.
 * Quotes within fields are escaped by doubling them.
 */
static void
WriteCsvField(BufferedOutputStream &writer, std::string_view field);

unsigned
LoadFlarmMessagingFile(BufferedReader &reader, FlarmMessagingDatabase &db)
{
  // Read and ignore header
  std::array<std::string_view, 5> header;
  ReadCsvRecord(reader, header);

  std::array<std::string_view, 5> cols;
  unsigned count = 0;

  while (ReadCsvRecord(reader, cols) == cols.size()) {
    MessagingRecord record;

    // id,pilot,plane_type,registration,callsign
    record.id = FlarmId::Parse(std::string(cols[0]).c_str(), nullptr);
    if (!record.id.IsDefined()) {
      continue;
    }

    record.pilot = std::string(cols[1]);
    record.plane_type = std::string(cols[2]);
    record.registration = std::string(cols[3]);
    record.callsign = std::string(cols[4]);

    db.Insert(record);
    count++;
  }

  return count;
}

void
SaveFlarmMessagingFile(BufferedOutputStream &writer, FlarmMessagingDatabase &db)
{
  // Write CSV header to match what LoadFlarmMessagingFile expects
  writer.Write("id,pilot,plane_type,registration,callsign\n");

  const std::lock_guard<SharedMutex> lock(db.mutex);

  for (const auto &i : db.map) {
    char id_buf[16];
    WriteCsvField(writer, i.second.id.Format(id_buf));
    writer.Write(',');
    WriteCsvField(writer, i.second.pilot);
    writer.Write(',');
    WriteCsvField(writer, i.second.plane_type);
    writer.Write(',');
    WriteCsvField(writer, i.second.registration);
    writer.Write(',');
    WriteCsvField(writer, i.second.callsign);
    writer.Write('\n');
  }
}

static void
WriteCsvField(BufferedOutputStream &writer, std::string_view field)
{
  const bool needs_quoting = field.find_first_of(",\"\n\r") != std::string_view::npos;
  
  if (needs_quoting) {
    writer.Write('"');
    for (char ch : field) {
      writer.Write(ch);
      if (ch == '"') {
        writer.Write('"');  // Escape quote by doubling
      }
    }
    writer.Write('"');
  } else {
    writer.Write(field);
  }
}
