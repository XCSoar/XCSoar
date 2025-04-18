// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficDatabases.hpp"
#include "util/StringCompare.hxx"

const TCHAR *
TrafficDatabases::FindNameById(FlarmId id) const noexcept
{
  // try to find flarm from userFile
  const TCHAR *name = flarm_names.Get(id);
  if (name != nullptr)
    return name;

  // try to find flarm from FlarmNet.org File
  const FlarmNetRecord *record = flarm_net.FindRecordById(id);
  if (record != nullptr)
    return record->callsign;

  return nullptr;
}

FlarmId
TrafficDatabases::FindIdByName(const TCHAR *name) const noexcept
{
  assert(!StringIsEmpty(name));

  // try to find flarm from userFile
  const FlarmId id = flarm_names.Get(name);
  if (id.IsDefined())
    return id;

  // try to find flarm from FlarmNet.org File
  const FlarmNetRecord *record = flarm_net.FindFirstRecordByCallSign(name);
  if (record != NULL)
    return record->GetId();

  return FlarmId::Undefined();
}

unsigned
TrafficDatabases::FindIdsByName(const TCHAR *name,
                                FlarmId *buffer, unsigned max) const noexcept
{
  assert(!StringIsEmpty(name));

  unsigned n = flarm_names.Get(name, buffer, max);
  if (n < max)
    n += flarm_net.FindIdsByCallSign(name, buffer + n, max - n);

  return n;
}
