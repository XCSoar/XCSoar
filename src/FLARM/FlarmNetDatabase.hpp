// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Id.hpp"
#include "FlarmNetRecord.hpp"

#include <map>
#include <tchar.h>

/**
 * An in-memory representation of the FlarmNet.org database.
 */
class FlarmNetDatabase {
  typedef std::map<FlarmId, FlarmNetRecord> RecordMap;
  RecordMap map;

public:
  bool IsEmpty() const noexcept {
    return map.empty();
  }

  void Clear() noexcept {
    map.clear();
  }

  void Insert(const FlarmNetRecord &record) noexcept;

  /**
   * Finds a FLARMNetRecord object based on the given FLARM id
   * @param id FLARM id
   * @return FLARMNetRecord object
   */
  [[gnu::pure]]
  const FlarmNetRecord *FindRecordById(FlarmId id) const noexcept {
    auto i = map.find(id);
    return i != map.end()
      ? &i->second
      : NULL;
  }

  /**
   * Finds a FLARMNetRecord object based on the given Callsign
   * @param cn Callsign
   * @return FLARMNetRecord object
   */
  [[gnu::pure]]
  const FlarmNetRecord *FindFirstRecordByCallSign(const char *cn) const noexcept;

  unsigned FindRecordsByCallSign(const char *cn,
                                 const FlarmNetRecord *array[],
                                 unsigned size) const noexcept;
  unsigned FindIdsByCallSign(const char *cn, FlarmId array[],
                             unsigned size) const noexcept;

  [[gnu::pure]]
  auto begin() const noexcept {
    return map.begin();
  }

  [[gnu::pure]]
  auto end() const noexcept {
    return map.end();
  }
};
