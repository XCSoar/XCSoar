/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Util/StringUtil.hpp"
#include "LocalPath.hpp"

#include <assert.h>
#include <windef.h> // for MAX_PATH

Source<char> *
OpenDataFile(const TCHAR *name)
{
  assert(name != NULL);
  assert(!StringIsEmpty(name));

  TCHAR path[MAX_PATH];
  LocalPath(path, name);

  FileSource *source = new FileSource(path);
  if (source == NULL)
    return NULL;

  if (source->error()) {
    delete source;
    return NULL;
  }

  return source;
}

TLineReader *
OpenDataTextFile(const TCHAR *name, ConvertLineReader::charset cs)
{
  assert(name != NULL);
  assert(!StringIsEmpty(name));

  TCHAR path[MAX_PATH];
  LocalPath(path, name);

  FileLineReader *reader = new FileLineReader(path, cs);
  if (reader == NULL)
    return NULL;

  if (reader->error()) {
    delete reader;
    return NULL;
  }

  return reader;
}

NLineReader *
OpenDataTextFileA(const TCHAR *name)
{
  assert(name != NULL);
  assert(!StringIsEmpty(name));

  TCHAR path[MAX_PATH];
  LocalPath(path, name);

  FileLineReaderA *reader = new FileLineReaderA(path);
  if (reader == NULL)
    return NULL;

  if (reader->error()) {
    delete reader;
    return NULL;
  }

  return reader;
}

TextWriter *
CreateDataTextFile(const TCHAR *name, bool append)
{
  assert(name != NULL);
  assert(!StringIsEmpty(name));

  TCHAR path[MAX_PATH];
  LocalPath(path, name);

  TextWriter *writer = new TextWriter(path, append);
  if (writer == NULL)
    return NULL;

  if (!writer->IsOpen()) {
    delete writer;
    return NULL;
  }

  return writer;
}
