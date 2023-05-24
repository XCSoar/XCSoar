// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FLARM/FlarmDetails.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"
#include "FLARM/FlarmId.hpp"
#include "util/StringCompare.hxx"

#include <cassert>

namespace FlarmDetails {

const FlarmNetRecord *
LookupRecord(FlarmId id)
{
  // try to find flarm from FlarmNet.org File
  if (traffic_databases == nullptr)
    return NULL;

  return traffic_databases->flarm_net.FindRecordById(id);
}

const TCHAR *
LookupCallsign(FlarmId id)
{
  if (traffic_databases == nullptr)
    return nullptr;

  return traffic_databases->FindNameById(id);
}

FlarmId
LookupId(const TCHAR *cn)
{
  assert(traffic_databases != nullptr);

  return traffic_databases->FindIdByName(cn);
}

bool
AddSecondaryItem(FlarmId id, const TCHAR *name)
{
  assert(id.IsDefined());
  assert(traffic_databases != nullptr);

  return traffic_databases->flarm_names.Set(id, name);
}

unsigned
FindIdsByCallSign(const TCHAR *cn, FlarmId array[],
                                unsigned size)
{
  assert(cn != NULL);
  assert(!StringIsEmpty(cn));
  assert(traffic_databases != nullptr);

  return traffic_databases->FindIdsByName(cn, array, size);
}

} // namespace FlarmDetails
