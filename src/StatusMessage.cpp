// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StatusMessage.hpp"
#include "util/StringAPI.hxx"
#include "util/Macros.hpp"

#include <cassert>

static constexpr StatusMessage default_status_messages[] = {
#include "Status_defaults.cpp"
};

[[gnu::pure]]
const StatusMessage &
FindStatusMessage(const char *key)
{
  assert(ARRAY_SIZE(default_status_messages) > 0);

  unsigned i = ARRAY_SIZE(default_status_messages) - 1;
  for (; i > 0; --i)
    if (StringIsEqual(key, default_status_messages[i].key))
      break;

  return default_status_messages[i];
}
