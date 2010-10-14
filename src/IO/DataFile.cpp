/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "StringUtil.hpp"
#include "LocalPath.hpp"

#include <assert.h>
#include <windef.h> // for MAX_PATH

Source<char> *
OpenDataFile(const TCHAR *name)
{
  assert(name != NULL);
  assert(!string_is_empty(name));

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
#ifdef _UNICODE
  assert(name != NULL);
  assert(!string_is_empty(name));

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
#else
  return OpenDataTextFileA(name);
#endif
}

NLineReader *
OpenDataTextFileA(const TCHAR *name)
{
  assert(name != NULL);
  assert(!string_is_empty(name));

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
  assert(!string_is_empty(name));

  TCHAR path[MAX_PATH];
  LocalPath(path, name);

  TextWriter *writer = new TextWriter(path, append);
  if (writer == NULL)
    return NULL;

  if (writer->error()) {
    delete writer;
    return NULL;
  }

  return writer;
}
