// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/OverwritingRingBuffer.hpp"

/**
 * A filter that stores a certain amount of samples and calculates the
 * derivative of the first and the last sample.  The motivation to
 * write this one was to answer the question "what was the average
 * task speed over the last 60 minutes?"
 */
template<unsigned N>
class DifferentialWindowFilter {
  struct Sample {
    double x, y;
  };

  OverwritingRingBuffer<Sample, N> buffer;

public:
  void Clear() noexcept {
    buffer.clear();
  }

  bool IsEmpty() const noexcept {
    return buffer.empty();
  }

  double GetFirstX() const noexcept {
    return buffer.peek().x;
  }

  double GetLastX() const noexcept {
    return buffer.last().x;
  }

  /**
   * Returns the difference between the first and the last X sample.
   * Must not be called on an empty object.
   */
  double GetDeltaX() const noexcept {
    return GetLastX() - GetFirstX();
  }

  /**
   * Same as GetDeltaX(), but returns -1 if the object is empty.
   */
  double GetDeltaXChecked() const noexcept {
    return IsEmpty() ? -1. : GetDeltaX();
  }

  double GetDeltaY() const noexcept {
    return buffer.last().y - buffer.peek().y;
  }

  /**
   * Add a new sample.
   */
  void Push(double x, double y) noexcept {
    buffer.push({x, y});
  }

  /**
   * Does this object have enough data to calculate the derivative of
   * at least the specified delta?
   */
  [[gnu::pure]]
  bool HasEnoughData(double min_delta_x) const noexcept {
    return !IsEmpty() && GetDeltaX() >= min_delta_x;
  }

  /**
   * Calculate the average dy/dx over the whole window.  This may only
   * be called after HasEnoughData() has returned true.
   */
  [[gnu::pure]]
  double DeriveAverage() const noexcept {
    auto delta_x = GetDeltaX();
    auto delta_y = GetDeltaY();
    return delta_y / delta_x;
  }
};
