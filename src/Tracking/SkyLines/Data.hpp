// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/Mutex.hxx"

#include <string>
#include <map>
#include <cstdint>

namespace SkyLinesTracking {

struct Data {
  mutable Mutex mutex;

  /**
   * A database of user-id to display-name.  An empty string means
   * the server has failed/refused to supply a name.
   */
  std::map<uint32_t, std::string> user_names;

  [[gnu::pure]]
  bool IsUserKnown(uint32_t id) const {
    return user_names.find(id) != user_names.end();
  }
};

} /* namespace SkyLinesTracking */
