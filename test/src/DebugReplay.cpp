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

#include "DebugReplay.hpp"
#include "DebugReplayIGC.hpp"
#include "DebugReplayNMEA.hpp"
#include "OS/Args.hpp"
#include "IO/FileLineReader.hpp"
#include "OS/PathName.hpp"
#include "Device/Register.hpp"
#include "ComputerSettings.hpp"

DebugReplay::DebugReplay(NLineReader *_reader)
  :reader(_reader), glide_polar(fixed(1))
{
  raw_basic.Reset();
  computed_basic.Reset();
  calculated.Reset();

  flying_computer.Reset();
}

DebugReplay::~DebugReplay()
{
  delete reader;
}

long
DebugReplay::Size() const
{
  return reader->GetSize();
}

long
DebugReplay::Tell() const
{
  return reader->Tell();
}

void
DebugReplay::Compute()
{
  computed_basic.Reset();
  (NMEAInfo &)computed_basic = raw_basic;

  FeaturesSettings features;
  features.nav_baro_altitude_enabled = true;
  computer.Fill(computed_basic, AtmosphericPressure::Standard(), features);

  computer.Compute(computed_basic, last_basic, last_basic, calculated);
  flying_computer.Compute(glide_polar.GetVTakeoff(),
                          computed_basic, calculated,
                          calculated.flight);
}

DebugReplay *
CreateDebugReplay(Args &args)
{
  if (!args.IsEmpty() && MatchesExtension(args.PeekNext(), ".igc")) {
    const char *input_file = args.ExpectNext();

    FileLineReaderA *reader = new FileLineReaderA(input_file);
    if (reader->error()) {
      delete reader;
      fprintf(stderr, "Failed to open %s\n", input_file);
      return NULL;
    }

    return new DebugReplayIGC(reader);
  }

  const tstring driver_name = args.ExpectNextT();

  const struct DeviceRegister *driver = FindDriverByName(driver_name.c_str());
  if (driver == NULL) {
    _ftprintf(stderr, _T("No such driver: %s\n"), driver_name.c_str());
    return NULL;
  }

  const char *input_file = args.ExpectNext();

  FileLineReaderA *reader = new FileLineReaderA(input_file);
  if (reader->error()) {
    delete reader;
    fprintf(stderr, "Failed to open %s\n", input_file);
    return NULL;
  }

  return new DebugReplayNMEA(reader, driver);
}
