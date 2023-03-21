// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FlarmId.hpp"
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
  bool IsEmpty() const {
    return map.empty();
  }

  void Clear() {
    map.clear();
  }

  void Insert(const FlarmNetRecord &record);

  /**
   * Finds a FLARMNetRecord object based on the given FLARM id
   * @param id FLARM id
   * @return FLARMNetRecord object
   */
  [[gnu::pure]]
  const FlarmNetRecord *FindRecordById(FlarmId id) const {
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
  const FlarmNetRecord *FindFirstRecordByCallSign(const TCHAR *cn) const;

  unsigned FindRecordsByCallSign(const TCHAR *cn,
                                 const FlarmNetRecord *array[],
                                 unsigned size) const;
  unsigned FindIdsByCallSign(const TCHAR *cn, FlarmId array[],
                             unsigned size) const;

  RecordMap::const_iterator begin() const {
    return map.begin();
  }

  RecordMap::const_iterator end() const {
    return map.end();
  }
};
