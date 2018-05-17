/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "FileReader.hxx"
#include "FileLineReader.hpp"
#include "ConvertLineReader.hpp"
#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "Util/StringCompare.hxx"

#include <assert.h>

std::unique_ptr<Reader>
OpenDataFile(const TCHAR *name)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);
  return std::make_unique<FileReader>(path);
}

std::unique_ptr<TLineReader>
OpenDataTextFile(const TCHAR *name, Charset cs)
{
  return std::make_unique<ConvertLineReader>(OpenDataTextFileA(name), cs);
}

std::unique_ptr<NLineReader>
OpenDataTextFileA(const TCHAR *name)
{
  assert(name != nullptr);
  assert(!StringIsEmpty(name));

  const auto path = LocalPath(name);
  return std::make_unique<FileLineReaderA>(path);
}
