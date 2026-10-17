// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/PagerWidget.hpp"

class TaskManagerDialog;
class OrderedTask;

class WeGlidePanel final : public PagerWidget {
public:
  enum Pages {
    PAGE_ACTIONS,
    PAGE_USER,
    PAGE_PUBLIC_DECLARED,
    PAGE_DAILY_COMPETITIONS,
    PAGE_RECENT_SCORES,
  };

  WeGlidePanel(TaskManagerDialog &dialog,
               std::unique_ptr<OrderedTask> &_active_task,
               bool *_task_modified) noexcept;

  /* virtual methods from class Widget */
  void ReClick() noexcept override;
  void Show(const PixelRect &rc) noexcept override;
};
