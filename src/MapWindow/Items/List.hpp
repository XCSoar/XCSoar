// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"

#include <cstddef>

struct MapItem;

class MapItemList: public StaticArray<MapItem *, 32>
{
public:
  ~MapItemList();

  void Sort();

  /**
   * Find the smallest airspace (by bounding box) in the list: all of
   * them contain the touched location, and the smallest is the one
   * drawn there, e.g. a CTR inside a TMA.  On a tie the first one in
   * list order wins.
   *
   * @return the index of that item, or size() if the list contains
   * no airspace
   */
  [[gnu::pure]]
  std::size_t FindSmallestAirspace() const noexcept;
};
