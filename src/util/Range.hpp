// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

/**
 * This class describes a range with two iterators: one pointing to
 * the first element and one pointing to the one after the last
 * element.  This is useful as return type.
 */
template<class I>
class Range {
  I begin_, end_;

public:
  Range() = default;
  Range(I _begin, I _end)
    :begin_(_begin), end_(_end) {}

  I begin() const {
    return begin_;
  }

  I end() const {
    return end_;
  }

  bool empty() const {
    return begin() == end();
  }
};
