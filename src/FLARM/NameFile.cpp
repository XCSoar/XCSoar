// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NameFile.hpp"
#include "NameDatabase.hpp"
#include "io/LineReader.hpp"
#include "io/BufferedOutputStream.hxx"
#include "util/StringStrip.hxx"

void
LoadFlarmNameFile(TLineReader &reader, FlarmNameDatabase &db)
{
  TCHAR *line;
  while ((line = reader.ReadLine()) != NULL) {
    TCHAR *endptr;
    FlarmId id = FlarmId::Parse(line, &endptr);
    if (!id.IsDefined())
      /* ignore malformed records */
      continue;

    if (endptr > line && endptr[0] == _T('=') && endptr[1] != _T('\0')) {
      TCHAR *Name = endptr + 1;
      StripRight(Name);
      if (!db.Set(id, Name))
        break; // cant add anymore items !
    }
  }
}

void
SaveFlarmNameFile(BufferedOutputStream &writer, FlarmNameDatabase &db)
{
  TCHAR id[16];

  for (const auto &i : db) {
    assert(i.id.IsDefined());

    writer.Write(i.id.Format(id));
    writer.Write('=');
    writer.Write(i.name.c_str());
    writer.Write('\n');
  }
}
