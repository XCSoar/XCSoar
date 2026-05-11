// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskEditMapPreviewWidget.hpp"
#include "TaskEditMapPreviewWindow.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/Window.hpp"

TaskEditMapPreviewWidget::TaskEditMapPreviewWidget(
  std::unique_ptr<TaskEditMapPreviewWindow> w) noexcept
  : WindowWidget(std::move(w)) {}

void
TaskEditMapPreviewWidget::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  auto &map = static_cast<TaskEditMapPreviewWindow &>(GetWindow());

  WindowStyle style;
  style.Hide();
  style.Border();

  map.Create(parent, rc, style);
}

PixelSize
TaskEditMapPreviewWidget::GetMinimumSize() const noexcept
{
  return {Layout::Scale(120), Layout::Scale(100)};
}

PixelSize
TaskEditMapPreviewWidget::GetMaximumSize() const noexcept
{
  return WidgetMaximumSizeUnbounded();
}
