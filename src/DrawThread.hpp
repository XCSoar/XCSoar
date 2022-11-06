/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2022 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
