// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "thread/RecursivelySuspensibleThread.hpp"

class GlueMapWindow;

/**
 * The DrawThread handles the rendering and drawing on the screen.
 * The Map and GaugeFLARM both are triggered on GPS updates synchronously, 
 * which is why they are both handled by this thread.  The GaugeVario is
 * triggered on vario data which may be faster than GPS updates, which is
 * why it is not handled by this thread.
 * 
 */
class DrawThread final : public RecursivelySuspensibleThread {
  /**
   * Is work pending?  This flag gets cleared by the thread as soon as
   * it starts working.
   */
  bool pending = true;

  /** Pointer to the MapWindow */
  GlueMapWindow &map;

public:
  DrawThread(GlueMapWindow &_map) noexcept
    :RecursivelySuspensibleThread("DrawThread"), map(_map) {}

  /**
   * Triggers a redraw.
   */
  void TriggerRedraw() noexcept {
    const std::lock_guard lock{mutex};
    pending = true;
    command_trigger.notify_one();
  }

protected:
  void Run() noexcept override;
};
