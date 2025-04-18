// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Rect.hpp"
#include "util/StaticArray.hxx"

/**
 * Simple code to prevent text writing over map city names.
 */
class LabelBlock {
  static constexpr unsigned SCREEN_HEIGHT = 4096;
  static constexpr unsigned BUCKET_SIZE = 64;
  static constexpr unsigned BUCKET_SHIFT = 7;
  static constexpr unsigned BUCKET_HEIGHT = 1 << BUCKET_SHIFT;
  static constexpr unsigned BUCKET_COUNT = SCREEN_HEIGHT / BUCKET_HEIGHT;

  /**
   * A bucket is responsible for hit tests in one horizontal section
   * of the screen.
   */
  class Bucket {
    typedef StaticArray<PixelRect, BUCKET_SIZE> BlockArray;
    BlockArray blocks;

  public:
    void Clear() noexcept {
      blocks.clear();
    }

    [[gnu::pure]]
    bool Check(const PixelRect rc) const noexcept;

    void Add(const PixelRect rc) noexcept {
      if (!blocks.full())
        blocks.append(rc);
    }
  };

  Bucket buckets[BUCKET_COUNT];

public:
  bool check(const PixelRect rc) noexcept;
  void reset() noexcept;
};
