// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/PagerWidget.hpp"

class TaskManagerDialog;
class OrderedTask;

class TaskMiscPanel final : public PagerWidget {
public:
  enum Pages {
    PAGE_ACTIONS,
    PAGE_LIST,
    PAGE_WEGLIDE_USER,
    PAGE_WEGLIDE_PUBLIC_DECLARED,
  };

  TaskMiscPanel(TaskManagerDialog &dialog,
                std::unique_ptr<OrderedTask> &_active_task,
                bool *_task_modified) noexcept;

  /* virtual methods from class Widget */
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};
