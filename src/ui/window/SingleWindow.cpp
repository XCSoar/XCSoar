// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SingleWindow.hpp"
#include "Form/Form.hpp"

namespace UI {

void
SingleWindow::AddDialog(WndForm *dialog) noexcept
{
  dialogs.push_front(dialog);
}

void
SingleWindow::RemoveDialog([[maybe_unused]] WndForm *dialog) noexcept
{
  assert(dialog == dialogs.front());

  dialogs.pop_front();
}

void
SingleWindow::CancelDialog() noexcept
{
  AssertThread();

  GetTopDialog().SetModalResult(mrCancel);
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
  TopWindow::OnResize(new_size);

  const PixelRect rc = GetClientRect();
  for (WndForm *dialog : dialogs)
    dialog->ReinitialiseLayout(rc);
}

} // namespace UI
