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
  /* >= so a Full dialog created before the first inset report (when
     the "safe" area was still the whole window) shrinks into the
     real safe area instead of staying edge-to-edge */
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

void
SingleWindow::OnResize(PixelSize new_size) noexcept
{
  /* Resize dialogs BEFORE calling TopWindow::OnResize, so they're at the
   * correct size when TopWindow::OnResize calls Expose(). This is especially
   * important for maximised dialogs (like the Fly/Simulator start
   * screen) which cover the safe area.
   *
   * Use the new_size to construct the client rect, since GetClientRect()
   * would return the old size at this point. */
  /* dialogs stay inside the safe area: a title or a button behind the
     display cutout or the home indicator is hard to read and hard to
     hit, and unlike the map they cannot be moved out of the way */
  const PixelRect rc = GetSafeAreaRect(new_size);
  for (WndForm *dialog : dialogs) {
    if (FillsDialogArea(*dialog, dialog_rect))
      /* the dialog filled the (old) safe area; grow or shrink it to
         the new one instead of only moving it, because
         ReinitialiseLayout() never resizes.  This happens during
         startup, where the Fly/Simulator dialog is created before
         the system reports the display cutout and the system bars. */
      dialog->Move(rc);
    else
      dialog->ReinitialiseLayout(rc);

    /* Invalidate dialog to ensure it's redrawn with the new layout */
    dialog->Invalidate();
  }

  dialog_rect = rc;
  
  /* Now resize the main window, which will call Expose() to redraw everything
   * including the resized dialogs. */
  TopWindow::OnResize(new_size);
}

} // namespace UI
