// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "File.hpp"

#include "Keys.hpp"
#include "Map.hpp"
#include "windef.h"
#include "boost/json/string.hpp"
#include "io/KeyValueFileReader.hpp"
#include "io/FileLineReader.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "io/KeyValueFileWriter.hpp"
#include "system/Path.hpp"
#include "util/StringAPI.hxx"

void
Profile::LoadFile(ProfileMap &map, Path path)
{
  FileLineReaderA reader(path);
  KeyValueFileReader kvreader(reader);
  KeyValuePair pair;


  while (kvreader.Read(pair)) {
    // migrate old AirspaceFile and AdditionalAirspaceFile field
    if (StringIsEqual(pair.key, "AirspaceFile") || StringIsEqual(
            pair.key, "AdditionalAirspaceFile")) {
      auto buffer = map.Get(ProfileKeys::AirspaceFileList);
      std::string airspace;
      if (buffer != nullptr) {
        airspace = std::string(buffer);
      }

      if (airspace.size() > 0)
        airspace += "|";
      airspace += pair.value;
      map.Set(ProfileKeys::AirspaceFileList, airspace.c_str());
      continue;
    }

    /* ignore the "Vega*" values; the Vega driver used to abuse the
       profile to pass messages between the driver and the user
       interface */
    if (!StringIsEqual(pair.key, "Vega", 4))
      map.Set(pair.key, pair.value);
  }
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

  /* profiles contain important data, so let's make sure everything
     has been written to permanent storage before we replace the old
     file */
  file.Sync();

  file.Commit();
}
