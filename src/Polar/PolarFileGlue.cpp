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

#include "Polar/PolarFileGlue.hpp"
#include "Parser.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"

bool
PolarGlue::LoadFromFile(PolarInfo &polar, NLineReader &reader)
{
  const char *line;
  while ((line = reader.ReadLine()) != nullptr)
    if (ParsePolar(polar, line))
      return true;

  return false;
}

void
PolarGlue::LoadFromFile(PolarInfo &polar, Path path)
{
  FileLineReaderA reader(path);
  LoadFromFile(polar, reader);
}

void
PolarGlue::SaveToFile(const PolarInfo &polar, BufferedOutputStream &writer)
{
  char buffer[256];
  FormatPolar(polar, buffer, 256);
  writer.Write(buffer);
  writer.Write('\n');
}

void
PolarGlue::SaveToFile(const PolarInfo &polar, Path path)
{
  FileOutputStream file(path);
  BufferedOutputStream writer(file);
  SaveToFile(polar, writer);
  writer.Flush();
  file.Commit();
}
