// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Details.hpp"
#include "Id.hpp"
#include "Global.hpp"
#include "TrafficDatabases.hpp"
#include "MessagingDatabase.hpp"
#include "MessagingRecord.hpp"
#include "Language/Language.hpp"
#include "util/Macros.hpp"
#include "util/StringCompare.hxx"

#include <cassert>
#include <utility>

// Weak reference to SaveFlarmMessagingPeriodic to avoid requiring Glue.cpp in all tests
extern void SaveFlarmMessagingPeriodic() noexcept __attribute__((weak));

namespace FlarmDetails {

const FlarmNetRecord *
LookupRecord(FlarmId id) noexcept
{
  // try to find flarm from FlarmNet.org File
  if (traffic_databases == nullptr)
    return NULL;

  return traffic_databases->flarm_net.FindRecordById(id);
}

const TCHAR *
LookupCallsign(FlarmId id) noexcept
{
  if (traffic_databases == nullptr)
    return nullptr;

  return traffic_databases->FindNameById(id);
}

FlarmId
LookupId(const TCHAR *cn) noexcept
{
  assert(traffic_databases != nullptr);

  return traffic_databases->FindIdByName(cn);
}

bool
AddSecondaryItem(FlarmId id, const TCHAR *name) noexcept
{
  assert(id.IsDefined());
  assert(traffic_databases != nullptr);

  return traffic_databases->flarm_names.Set(id, name);
}

void
StoreMessagingRecord(const MessagingRecord &record) noexcept
{
  if (traffic_databases == nullptr)
    return;

  traffic_databases->flarm_messages.Update(record);

  if (SaveFlarmMessagingPeriodic != nullptr)
    SaveFlarmMessagingPeriodic();
}

unsigned
FindIdsByCallSign(const TCHAR *cn, FlarmId array[], unsigned size) noexcept
{
  assert(cn != NULL);
  assert(!StringIsEmpty(cn));
  assert(traffic_databases != nullptr);

  return traffic_databases->FindIdsByName(cn, array, size);
}

ResolvedInfo
ResolveInfo(FlarmId id) noexcept
{
  ResolvedInfo out;
  if (traffic_databases == nullptr)
    return out;

  const auto msg = std::as_const(traffic_databases->flarm_messages).FindRecordById(id);
  const auto *net = traffic_databases->flarm_net.FindRecordById(id);

  out.callsign = traffic_databases->FindNameById(id);

  static thread_local StaticString<256> pilot_buf, plane_buf, reg_buf, airfield_buf;

  if (msg.has_value()) {
    out.source = ResolvedSource::MESSAGING;

    out.pilot = msg->Format(pilot_buf, msg->pilot);
    out.plane_type = msg->Format(plane_buf, msg->plane_type);
    out.registration = msg->Format(reg_buf, msg->registration);

    if (msg->frequency.IsDefined())
      out.frequency = msg->frequency;
  } else if (net != nullptr) {
    out.source = ResolvedSource::FLARMNET;

    out.pilot = net->Format(pilot_buf, net->pilot.c_str());
    out.plane_type = net->Format(plane_buf, net->plane_type.c_str());
    out.registration = net->Format(reg_buf, net->registration.c_str());

    const TCHAR *af = net->Format(airfield_buf, net->airfield.c_str());
    if (af != nullptr && (out.registration == nullptr || !StringIsEqual(af, out.registration)))
      out.airfield = af;

    if (net->frequency.IsDefined())
      out.frequency = net->frequency;
  }

  const TCHAR *user_callsign = traffic_databases->flarm_names.Get(id);
  if (user_callsign != nullptr)
    out.callsign = user_callsign;

  return out;
}

static const TCHAR *const resolved_source_strings[] = {
  N_("Unresolved"),
  N_("Flarm Messaging"),
  N_("FLARMnet"),
};

const TCHAR *ToString(ResolvedSource source) noexcept
{
  unsigned i = (unsigned)source;
  const TCHAR *text = i < ARRAY_SIZE(resolved_source_strings)
    ? resolved_source_strings[i]
    : N_("Unknown");

  return gettext(text);
}

static_assert(ARRAY_SIZE(resolved_source_strings) == static_cast<unsigned>(ResolvedSource::COUNT));

} // namespace FlarmDetails
