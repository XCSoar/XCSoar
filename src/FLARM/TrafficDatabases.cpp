// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficDatabases.hpp"
#include "MessagingRecord.hpp"
#include "util/StringCompare.hxx"
#include "util/StaticString.hxx"

const char *
TrafficDatabases::FindNameById(FlarmId id) const noexcept
{
  // try to find flarm from userFile (secondary names)
  const char *name = flarm_names.Get(id);
  if (name != nullptr)
    return name;

  // try to find flarm from PFLAM messaging database
  const auto msg_record = flarm_messages.FindRecordById(id);
  if (msg_record.has_value() && !msg_record->callsign.empty()) {
    static thread_local StaticString<256> callsign_buf;
    const char *cs = msg_record->Format(callsign_buf, msg_record->callsign);

    if (cs != nullptr && cs[0] != 0)
      return cs;
  }

  // try to find flarm from FlarmNet.org File
  const FlarmNetRecord *record = flarm_net.FindRecordById(id);
  if (record != nullptr)
    return record->callsign;

  return nullptr;
}

FlarmId
TrafficDatabases::FindIdByName(const char *name) const noexcept
{
  assert(!StringIsEmpty(name));

  // try to find flarm from userFile
  const FlarmId id = flarm_names.Get(name);
  if (id.IsDefined())
    return id;

  // try to find flarm from FlarmNet.org File
  const FlarmNetRecord *record = flarm_net.FindFirstRecordByCallSign(name);
  if (record != NULL)
    return record->id;

  return FlarmId::Undefined();
}

unsigned
TrafficDatabases::FindIdsByName(const char *name,
                                FlarmId *buffer, unsigned max) const noexcept
{
  assert(!StringIsEmpty(name));

  unsigned n = flarm_names.Get(name, buffer, max);
  if (n < max)
    n += flarm_net.FindIdsByCallSign(name, buffer + n, max - n);

  return n;
}
