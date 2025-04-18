// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
struct TaskLook;
struct TaskSummary;

class TaskProgressRenderer {
  const TaskLook &look;

public:
  TaskProgressRenderer(const TaskLook &_look):look(_look) {}

  void Draw(const TaskSummary &summary, Canvas &canvas,
            const PixelRect &rc, bool inverse);
};
