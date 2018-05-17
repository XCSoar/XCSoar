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

#include "File.hpp"
#include "Map.hpp"
#include "IO/KeyValueFileReader.hpp"
#include "IO/FileLineReader.hpp"
#include "IO/FileOutputStream.hxx"
#include "IO/BufferedOutputStream.hxx"
#include "IO/KeyValueFileWriter.hpp"
#include "OS/Path.hpp"
#include "Util/StringAPI.hxx"

void
Profile::LoadFile(ProfileMap &map, Path path)
{
  FileLineReaderA reader(path);
  KeyValueFileReader kvreader(reader);
  KeyValuePair pair;
  while (kvreader.Read(pair))
    /* ignore the "Vega*" values; the Vega driver used to abuse the
       profile to pass messages between the driver and the user
       interface */
    if (!StringIsEqual(pair.key, "Vega", 4))
      map.Set(pair.key, pair.value);
}

void
Profile::SaveFile(const ProfileMap &map, Path path)
{
  FileOutputStream file(path);
  BufferedOutputStream buffered(file);
  KeyValueFileWriter kvwriter(buffered);

  for (const auto &i : map)
    kvwriter.Write(i.first.c_str(), i.second.c_str());

  buffered.Flush();
  file.Commit();
}
