/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "co/InvokeTask.hxx"
#include "event/Loop.hxx"
#include "event/DeferEvent.hxx"

class CoInstance {
  EventLoop event_loop;

  Co::InvokeTask invoke_task;

  DeferEvent defer_start{event_loop, BIND_THIS_METHOD(OnDeferredStart)};

  std::exception_ptr error;

public:
  auto &GetEventLoop() noexcept {
    return event_loop;
  }

  void Run(Co::InvokeTask &&_task) {
    invoke_task = std::move(_task);
    defer_start.Schedule();
    event_loop.Run();
    if (error)
      std::rethrow_exception(error);
  }

private:
  void OnCompletion(std::exception_ptr _error) noexcept {
    error = std::move(_error);
    event_loop.Break();
  }

  void OnDeferredStart() noexcept {
    invoke_task.Start(BIND_THIS_METHOD(OnCompletion));
  }
};
