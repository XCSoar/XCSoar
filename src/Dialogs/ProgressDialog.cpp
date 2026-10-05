// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressDialog.hpp"
#include "Look/DialogLook.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"

#include <cassert>

using namespace UI;

static PixelRect
GetCancelButtonRect(const PixelRect &client_rc) noexcept
{
  PixelRect rc = client_rc;
  rc.left = rc.right - Layout::Scale(75);
  rc.bottom = rc.top + Layout::GetMaximumControlHeight();
  return rc;
}

ProgressDialog::ProgressDialog(SingleWindow &parent,
                               const DialogLook &dialog_look,
                               const char *caption)
  :WndForm(parent, dialog_look, parent.GetDialogRect(), caption),
   progress(GetClientAreaWindow())
{
  SetFillsClient(true);

  auto layout_client = [this]() noexcept {
    const PixelRect rc = GetClientAreaWindow().GetClientRect();
    progress.Move(rc);
    if (cancel_button.IsDefined())
      cancel_button.Move(GetCancelButtonRect(rc));
  };
  SetClientLayoutFunction(layout_client);
  layout_client();
}

void
ProgressDialog::ReinitialiseLayout([[maybe_unused]] const PixelRect &rc) noexcept
{
  /* Stay edge-to-edge on the client; the safe-area rect is only for
     ordinary dialogs. */
  Move(GetMainWindow().GetClientRect());
}

void
ProgressDialog::AddCancelButton(std::function<void()> &&callback)
{
  assert(!cancel_button.IsDefined());

  WindowStyle style;
  style.TabStop();

  const PixelRect rc = GetCancelButtonRect(client_area.GetClientRect());

  cancel_button.Create(client_area, GetLook().button,
                       _("Cancel"), rc, style,
                       [this](){ SetModalResult(mrCancel); });
  cancel_button.BringToTop();

  cancel_callback = std::move(callback);
}

void
ProgressDialog::SetModalResult(int id) noexcept
{
  if (id == mrCancel && cancel_callback)
    cancel_callback();
  else
    WndForm::SetModalResult(id);
}
