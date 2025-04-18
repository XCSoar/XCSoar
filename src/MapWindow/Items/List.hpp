// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"

struct MapItem;

class MapItemList: public StaticArray<MapItem *, 32>
{
public:
  ~MapItemList();

  void Sort();
};
