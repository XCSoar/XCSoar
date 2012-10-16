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

#include "Polar/PolarFileGlue.hpp"
#include "Parser.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/TextWriter.hpp"

bool
PolarGlue::LoadFromFile(PolarInfo &polar, TLineReader &reader)
{
  const TCHAR *line;
  while ((line = reader.ReadLine()) != NULL)
    if (ParsePolar(polar, line))
      return true;

  return false;
}

bool
PolarGlue::LoadFromFile(PolarInfo &polar, const TCHAR* path)
{
  FileLineReader *reader = new FileLineReader(path);
  if (reader == NULL)
    return false;

  if (reader->error()) {
    delete reader;
    return false;
  }

  LoadFromFile(polar, *reader);
  delete reader;
  return true;
}

bool
PolarGlue::SaveToFile(const PolarInfo &polar, TextWriter &writer)
{
  TCHAR buffer[256];
  FormatPolar(polar, buffer, 256);
  return writer.WriteLine(buffer);
}

bool
PolarGlue::SaveToFile(const PolarInfo &polar, const TCHAR* path)
{
  TextWriter writer(path);
  if (!writer.IsOpen())
    return false;

  return SaveToFile(polar, writer);
}
