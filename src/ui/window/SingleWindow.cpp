// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SingleWindow.hpp"
#include "Form/Form.hpp"

#include <algorithm>

namespace UI {

void
SingleWindow::AddDialog(WndForm *dialog) noexcept
{
  dialogs.push_front(dialog);
  OnDialogChanged();
  dialog->ReinitialiseLayout(GetDialogRect());
}

void
SingleWindow::RemoveDialog([[maybe_unused]] WndForm *dialog) noexcept
{
  assert(dialog == dialogs.front());

  dialogs.pop_front();
  OnDialogChanged();
}

PixelRect
SingleWindow::GetDialogRect(PixelRect rc) const noexcept
{
  rc.bottom -= std::min(dialog_bottom_margin, rc.GetHeight() / 2);
  return rc;
}

PixelRect
SingleWindow::GetDialogRect() const noexcept
{
  return GetDialogRect(GetClientRect());
}

void
SingleWindow::SetDialogOverlay(Window *window, unsigned bottom_margin) noexcept
{
  dialog_overlay = window;
  if (window == nullptr)
    bottom_margin = 0;

  if (dialog_bottom_margin == bottom_margin)
    return;

  dialog_bottom_margin = bottom_margin;
  ReinitialiseDialogs();
  Invalidate();
}

void
SingleWindow::ReinitialiseDialogs() noexcept
{
  const auto rc = GetDialogRect();
  for (auto *dialog : dialogs)
    dialog->ReinitialiseLayout(rc);
}

void
SingleWindow::CancelDialog() noexcept
{
  AssertThread();

  GetTopDialog().SetModalResult(mrCancel);
}

void
SingleWindow::BringToTopBelowDialogs(Window &window) noexcept
{
  WndForm *bottom_dialog = nullptr;
  for (auto *dialog : dialogs)
    bottom_dialog = dialog;

  if (bottom_dialog != nullptr)
    window.PlaceBelow(*bottom_dialog);
  else
    window.BringToTop();
}

bool
SingleWindow::OnClose() noexcept
{
  if (!dialogs.empty()) {
    /* close the current dialog instead of the main window */
    CancelDialog();
    return true;
  }

  return TopWindow::OnClose();
}

void
SingleWindow::OnDestroy() noexcept
{
  TopWindow::OnDestroy();
  PostQuit();
}

void
SingleWindow::OnResize(PixelSize new_size) noexcept
{
  /* Resize dialogs BEFORE calling TopWindow::OnResize, so they're at the
   * correct size when TopWindow::OnResize calls Expose(). This is especially
   * important for fullscreen dialogs (like dlgSimulatorPrompt) which cover
   * the entire window.
   * 
   * Use the new_size to construct the client rect, since GetClientRect()
   * would return the old size at this point. */
  const auto rc = GetDialogRect(PixelRect(PixelPoint(0, 0), new_size));
  for (WndForm *dialog : dialogs) {
    dialog->ReinitialiseLayout(rc);
    /* Invalidate dialog to ensure it's redrawn with the new layout */
    dialog->Invalidate();
  }
  
  /* Now resize the main window, which will call Expose() to redraw everything
   * including the resized dialogs. */
  TopWindow::OnResize(new_size);
}

} // namespace UI
