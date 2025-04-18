// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
  This code courtesy of unknown author at
  http://stackoverflow.com/questions/3666387/c-priority-queue-underlying-vector-container-capacity-resize
*/

#pragma once

#include <queue>

template<class T, class Container, class Compare>
class reservable_priority_queue:
  public std::priority_queue<T, Container, Compare>
{
public:
  typedef typename std::priority_queue<T, Container, Compare>::size_type size_type;
  reservable_priority_queue(size_type capacity = 0) {
    reserve(capacity);
  }

  void reserve(size_type capacity) {
    this->c.reserve(capacity);
  }

  size_type capacity() const {
    return this->c.capacity();
  }

  void clear() {
    this->c.clear();
  }
};
