// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BigThermalAssistantWindow.hpp"
#include "Input/InputEvents.hpp"
#include "Screen/Layout.hpp"

bool
BigThermalAssistantWindow::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  StopDragging();
  InputEvents::ShowMenu();
  return true;
}

bool
BigThermalAssistantWindow::OnMouseDown(PixelPoint p) noexcept
{
  if (!dragging) {
    dragging = true;
    SetCapture();
    gestures.Start(p, Layout::Scale(20));
  }

  return true;
}

bool
BigThermalAssistantWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  if (dragging) {
    StopDragging();

    const char *gesture = gestures.Finish();
    if (gesture && InputEvents::processGesture(gesture))
      return true;
  }

  return false;
}

bool
BigThermalAssistantWindow::OnMouseMove(PixelPoint p,
                                       [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging)
    gestures.Update(p);

  return true;
}

void
BigThermalAssistantWindow::OnCancelMode() noexcept
{
  ThermalAssistantWindow::OnCancelMode();
  StopDragging();
}

bool
BigThermalAssistantWindow::OnKeyDown(unsigned key_code) noexcept
{
  return InputEvents::processKey(key_code);
}
