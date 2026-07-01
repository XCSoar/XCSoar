// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficDatabases.hpp"
#include "MessagingRecord.hpp"
#include "util/StringCompare.hxx"
#include "util/StaticString.hxx"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE && defined(__arm__)
#include <new>
#include <pthread.h>

namespace {

pthread_key_t callsign_buffer_key;
bool callsign_buffer_key_valid;
pthread_once_t callsign_buffer_key_once = PTHREAD_ONCE_INIT;

void
DeleteCallsignBuffer(void *p) noexcept
{
  delete static_cast<StaticString<256> *>(p);
}

void
CreateCallsignBufferKey() noexcept
{
  callsign_buffer_key_valid =
    pthread_key_create(&callsign_buffer_key, DeleteCallsignBuffer) == 0;
}

StaticString<256> *
GetCallsignBuffer() noexcept
{
  pthread_once(&callsign_buffer_key_once, CreateCallsignBufferKey);
  if (!callsign_buffer_key_valid)
    return nullptr;

  auto *buffer = static_cast<StaticString<256> *>(
    pthread_getspecific(callsign_buffer_key));
  if (buffer != nullptr)
    return buffer;

  buffer = new (std::nothrow) StaticString<256>();
  if (buffer == nullptr)
    return nullptr;

  if (pthread_setspecific(callsign_buffer_key, buffer) != 0) {
    delete buffer;
    return nullptr;
  }

  return buffer;
}

} // namespace
#endif

const char *
TrafficDatabases::FindNameById(FlarmId id) const noexcept
{
  if (!id.IsDefined())
    return nullptr;

  // try to find flarm from userFile (secondary names)
  const char *name = flarm_names.Get(id);
  if (name != nullptr)
    return name;

  // try to find flarm from PFLAM messaging database
  const auto msg_record = flarm_messages.FindRecordById(id);
  if (msg_record.has_value() && !msg_record->callsign.empty()) {
#if defined(__APPLE__) && TARGET_OS_IPHONE && defined(__arm__)
    auto *callsign_buf = GetCallsignBuffer();
    if (callsign_buf != nullptr) {
#else
    static thread_local StaticString<256> callsign_buf;
#endif
    const char *cs = msg_record->Format(
#if defined(__APPLE__) && TARGET_OS_IPHONE && defined(__arm__)
      *callsign_buf,
#else
      callsign_buf,
#endif
      msg_record->callsign);

    if (cs != nullptr && cs[0] != 0)
      return cs;
#if defined(__APPLE__) && TARGET_OS_IPHONE && defined(__arm__)
    }
#endif
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
