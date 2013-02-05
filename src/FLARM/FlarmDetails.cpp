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

#include "FLARM/FlarmDetails.hpp"
#include "FLARM/FlarmId.hpp"
#include "NameDatabase.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StaticString.hpp"
#include "Util/TrivialArray.hpp"
#include "LogFile.hpp"
#include "LocalPath.hpp"
#include "FLARM/FlarmNet.hpp"
#include "IO/DataFile.hpp"
#include "IO/TextWriter.hpp"

#include <assert.h>

static FlarmNameDatabase *flarm_names;

void
FlarmDetails::Load()
{
  LogFormat("FlarmDetails::Load");

  LoadSecondary();
  LoadFLARMnet();
}

void
FlarmDetails::LoadFLARMnet()
{
  NLineReader *reader = OpenDataTextFileA(_T("data.fln"));
  if (reader == NULL)
    return;

  unsigned num_records = FlarmNet::LoadFile(*reader);
  delete reader;

  if (num_records > 0)
    LogFormat("%u FLARMnet ids found", num_records);
}

static void
LoadSecondaryFile(TLineReader &reader, FlarmNameDatabase &db)
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
      if (!FlarmDetails::AddSecondaryItem(id, Name))
        break; // cant add anymore items !
    }
  }
}

void
FlarmDetails::LoadSecondary()
{
  LogFormat("OpenFLARMDetails");

  // if (FLARM Details already there) delete them;
  delete flarm_names;
  flarm_names = new FlarmNameDatabase();

  TLineReader *reader = OpenDataTextFile(_T("xcsoar-flarm.txt"));
  if (reader != NULL) {
    LoadSecondaryFile(*reader, *flarm_names);
    delete reader;
  }
}

void
FlarmDetails::SaveSecondary()
{
  assert(flarm_names != nullptr);

  TextWriter *writer = CreateDataTextFile(_T("xcsoar-flarm.txt"));
  if (writer == NULL)
    return;

  TCHAR id[16];

  for (const auto &i : *flarm_names) {
    assert(i.id.IsDefined());

    writer->FormatLine(_T("%s=%s"),
                       i.id.Format(id),
                       i.name.c_str());
  }

  delete writer;
}

const FlarmRecord *
FlarmDetails::LookupRecord(FlarmId id)
{
  // try to find flarm from FlarmNet.org File
  return FlarmNet::FindRecordById(id);
}

const TCHAR *
FlarmDetails::LookupCallsign(FlarmId id)
{
  assert(flarm_names != nullptr);

  // try to find flarm from userFile
  const TCHAR *name = flarm_names->Get(id);
  if (name != nullptr)
    return name;

  // try to find flarm from FlarmNet.org File
  const FlarmRecord *record = FlarmNet::FindRecordById(id);
  if (record != NULL)
    return record->callsign;

  return NULL;
}

FlarmId
FlarmDetails::LookupId(const TCHAR *cn)
{
  // try to find flarm from userFile
  assert(flarm_names != nullptr);

  FlarmId id = flarm_names->Get(cn);
  if (id.IsDefined())
    return id;

  // try to find flarm from FlarmNet.org File
  const FlarmRecord *record = FlarmNet::FindFirstRecordByCallSign(cn);
  if (record != NULL)
    return record->GetId();

  return FlarmId::Undefined();
}

bool
FlarmDetails::AddSecondaryItem(FlarmId id, const TCHAR *name)
{
  assert(flarm_names != nullptr);

  if (!id.IsDefined())
    /* ignore malformed records */
    return false;

  return flarm_names->Set(id, name);
}

unsigned
FlarmDetails::FindIdsByCallSign(const TCHAR *cn, FlarmId array[],
                                unsigned size)
{
  assert(cn != NULL);
  assert(flarm_names != nullptr);

  if (StringIsEmpty(cn))
    return 0;

  return flarm_names->Get(cn, array, size);
}
