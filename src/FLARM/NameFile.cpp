// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NameFile.hpp"
#include "NameDatabase.hpp"
#include "io/BufferedReader.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/StringConverter.hpp"
#include "util/StringStrip.hxx"

void
LoadFlarmNameFile(BufferedReader &reader, FlarmNameDatabase &db)
{
  StringConverter string_converter{Charset::UTF8};

  char *line;
  while ((line = reader.ReadLine()) != nullptr) {
    char *endptr;
    FlarmId id = FlarmId::Parse(line, &endptr);
    if (!id.IsDefined())
      /* ignore malformed records */
      continue;

    if (endptr > line && endptr[0] == '=' && endptr[1] != '\0') {
      char *name = endptr + 1;
      StripRight(name);
      if (!db.Set(id, string_converter.Convert(name)))
        break; // cant add anymore items !
    }
  }
}

void
SaveFlarmNameFile(BufferedOutputStream &writer, FlarmNameDatabase &db)
{
  char id[16];

  for (const auto &i : db) {
    assert(i.id.IsDefined());

    writer.Write(i.id.Format(id));
    writer.Write('=');
    writer.Write(i.name);
    writer.Write('\n');
  }
}
