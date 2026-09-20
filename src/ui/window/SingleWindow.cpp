// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SingleWindow.hpp"
#include "Form/Form.hpp"

namespace UI {

void
SingleWindow::AddDialog(WndForm *dialog) noexcept
{
  dialogs.push_front(dialog);

  /* remember what a maximised dialog looks like right now, so
     OnResize() can tell one apart later */
  dialog_rect = GetSafeAreaRect();
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

/**
 * Does this dialog fill the whole area that is available to dialogs?
 *
 * WndForm::IsMaximised() answers the same question, but calling it
 * here would not link: #SingleWindow is part of the screen library,
 * which comes before the form library, and unlike WndForm's virtual
 * methods a non-virtual one is referenced by name.
 */
[[gnu::pure]]
static bool
FillsDialogArea(const Window &dialog, const PixelRect &dialog_rect) noexcept
{
  const PixelSize size = dialog.GetSize();
  const PixelSize available = dialog_rect.GetSize();
  /* >= so a Full dialog created before the first inset report shrinks
     into the real safe area instead of staying edge-to-edge */
  return size.width >= available.width && size.height >= available.height;
}

bool
SingleWindow::HasMaximisedDialog() const noexcept
{
  /* not just the top dialog: a small one opened on top of a maximised
     dialog (e.g. the settings of the analysis dialog) leaves the
     maximised one covering the screen */
  for (const WndForm *dialog : dialogs)
    if (FillsDialogArea(*dialog, dialog_rect))
      return true;

  return false;
}

bool
SingleWindow::HasFullScreenDialog() const noexcept
{
  const PixelRect rc = GetClientRect();
  for (const WndForm *dialog : dialogs)
    if (FillsDialogArea(*dialog, rc))
      return true;

  return false;
}

void
SingleWindow::OnResize(PixelSize new_size) noexcept
{
  /* Resize dialogs before TopWindow::OnResize so they have the right
     size when Expose() runs.  GetClientRect() still returns the old
     size here, so build the rects from new_size.

     Dialogs stay in the safe area.  The Fly/Simulator start screen is
     the exception: it fills the client so its gradient can paint
     edge to edge, and only its controls follow the insets. */
  const PixelRect full_rc{new_size};
  const PixelRect rc = GetSafeAreaRect(new_size);
  for (WndForm *dialog : dialogs) {
    if (FillsDialogArea(*dialog, full_rc)) {
      dialog->Move(full_rc);
      dialog->ForceLayout();
    } else if (FillsDialogArea(*dialog, dialog_rect))
      /* filled the previous safe area; grow or shrink with it.
         ReinitialiseLayout() never resizes. */
      dialog->Move(rc);
    else
      dialog->ReinitialiseLayout(rc);

    dialog->Invalidate();
  }

  dialog_rect = rc;

  TopWindow::OnResize(new_size);
}

} // namespace UI
