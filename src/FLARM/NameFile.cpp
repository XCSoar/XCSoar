/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "NameFile.hpp"
#include "NameDatabase.hpp"
#include "IO/TextWriter.hpp"
#include "IO/LineReader.hpp"

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
      TrimRight(Name);
      if (!db.Set(id, Name))
        break; // cant add anymore items !
    }
  }
}

void
SaveFlarmNameFile(TextWriter &writer, FlarmNameDatabase &db)
{
  TCHAR id[16];

  for (const auto &i : db) {
    assert(i.id.IsDefined());

    writer.FormatLine(_T("%s=%s"),
                       i.id.Format(id),
                       i.name.c_str());
  }
}
