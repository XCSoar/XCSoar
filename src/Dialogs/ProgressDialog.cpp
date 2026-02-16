// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressDialog.hpp"
#include "Look/DialogLook.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"

#include <cassert>

using namespace UI;

ProgressDialog::ProgressDialog(SingleWindow &parent,
                               const DialogLook &dialog_look,
                               const char *caption)
  :WndForm(parent, dialog_look, parent.GetClientRect(), caption),
   progress(GetClientAreaWindow())
{
}

void
ProgressDialog::AddCancelButton(std::function<void()> &&callback)
{
  assert(!cancel_button.IsDefined());

  WindowStyle style;
  style.TabStop();

  PixelRect rc = client_area.GetClientRect();
  rc.right -= Layout::Scale(2);
  rc.left = rc.right - Layout::Scale(78);
  rc.top += Layout::Scale(2);
  rc.bottom = rc.top + Layout::Scale(35);

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
