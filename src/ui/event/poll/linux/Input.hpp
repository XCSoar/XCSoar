// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "event/PipeEvent.hxx"
#include "Math/Point2D.hpp"

namespace UI {

class EventQueue;
class MergeMouse;
struct Event;

/**
 * A driver for Linux input devices (/dev/input/event*).
 */
class LinuxInputDevice final {
  typedef IntPoint2D Position;

  EventQueue &queue;

  MergeMouse &merge;

  int min_x, max_x, min_y, max_y;

  /**
   * The position being edited.  Upon EV_SYN, it will be copied to
   * #moved_position if #moving is true.
   */
  Position edit_position;

  /**
   * The position published by Generate().
   */
  Position public_position;

  int rel_x, rel_y, rel_wheel;

  bool down;

  /**
   * Was #edit_position modified?
   */
  bool moving;

  /**
   * Was the finger pressed or released, but not yet committed with
   * EV_SYN/SYN_REPORT?
   */
  bool pressing, releasing;

  bool is_pointer;

  PipeEvent event;

public:
  LinuxInputDevice(EventQueue &_queue, MergeMouse &_merge) noexcept;

  ~LinuxInputDevice() noexcept {
    Close();
  }

  bool Open(const char *path) noexcept;
  void Close() noexcept;

  bool IsOpen() const noexcept {
    return event.IsDefined();
  }

private:
  void Read() noexcept;

  void OnSocketReady(unsigned events) noexcept;
};

} // namespace UI
