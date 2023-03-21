// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/FlarmDetails.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"
#include "FLARM/FlarmId.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

const FlarmNetRecord *
FlarmDetails::LookupRecord(FlarmId id)
{
  // try to find flarm from FlarmNet.org File
  if (traffic_databases == nullptr)
    return NULL;

  return traffic_databases->flarm_net.FindRecordById(id);
}

const TCHAR *
FlarmDetails::LookupCallsign(FlarmId id)
{
  if (traffic_databases == nullptr)
    return nullptr;

  return traffic_databases->FindNameById(id);
}

FlarmId
FlarmDetails::LookupId(const TCHAR *cn)
{
  assert(traffic_databases != nullptr);

  return traffic_databases->FindIdByName(cn);
}

bool
FlarmDetails::AddSecondaryItem(FlarmId id, const TCHAR *name)
{
  assert(id.IsDefined());
  assert(traffic_databases != nullptr);

  return traffic_databases->flarm_names.Set(id, name);
}

unsigned
FlarmDetails::FindIdsByCallSign(const TCHAR *cn, FlarmId array[],
                                unsigned size)
{
  assert(cn != NULL);
  assert(!StringIsEmpty(cn));
  assert(traffic_databases != nullptr);

  return traffic_databases->FindIdsByName(cn, array, size);
}
