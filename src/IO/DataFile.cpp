/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "DataFile.hpp"
#include "FileLineReader.hpp"
#include "TextWriter.hpp"
#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "Util/StringCompare.hxx"

#include <assert.h>

Source<char> *
OpenDataFile(const TCHAR *name, Error &error)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);

  FileSource *source = new FileSource(path, error);
  if (source->error()) {
    delete source;
    return nullptr;
  }

  return source;
}

TLineReader *
OpenDataTextFile(const TCHAR *name, Error &error, Charset cs)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);

  FileLineReader *reader = new FileLineReader(path, error, cs);
  if (reader->error()) {
    delete reader;
    return nullptr;
  }

  return reader;
}

NLineReader *
OpenDataTextFileA(const TCHAR *name, Error &error)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);

  FileLineReaderA *reader = new FileLineReaderA(path, error);
  if (reader->error()) {
    delete reader;
    return nullptr;
  }

  return reader;
}

TextWriter *
CreateDataTextFile(const TCHAR *name, bool append)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);

  TextWriter *writer = new TextWriter(path, append);
  if (writer == nullptr)
    return nullptr;

  if (!writer->IsOpen()) {
    delete writer;
    return nullptr;
  }

  return writer;
}
