// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct PixelRect;
class Canvas;
struct DerivedInfo;
struct FinalGlideBarLook;
struct TaskLook;
struct GlideSettings;

class FinalGlideBarRenderer {
  const FinalGlideBarLook &look;
  const TaskLook &task_look;

public:
  FinalGlideBarRenderer(const FinalGlideBarLook &_look,
                        const TaskLook &_task_look)
    :look(_look), task_look(_task_look) {}

  const FinalGlideBarLook &GetLook() const {
    return look;
  }

  void Draw(Canvas &canvas, const PixelRect &rc,
            const DerivedInfo &calculated,
            const GlideSettings &glide_settings,
            const bool final_glide_bar_mc0_enabled) const;
};
