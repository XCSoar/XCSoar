// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/WindowWidget.hpp"

#include <memory>

class TaskEditMapPreviewWindow;

/**
 * #WindowWidget host for #TaskEditMapPreviewWindow (task manager embedded map).
 */
class TaskEditMapPreviewWidget final : public WindowWidget {
public:
  explicit TaskEditMapPreviewWidget(
    std::unique_ptr<TaskEditMapPreviewWindow> w) noexcept;

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;

  PixelSize GetMinimumSize() const noexcept override;
  PixelSize GetMaximumSize() const noexcept override;
};
