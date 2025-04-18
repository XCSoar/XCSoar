// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/Widget.hpp"
#include "Language/Language.hpp"
#include "ui/window/SingleWindow.hpp"
#include "Screen/Layout.hpp"

using namespace UI;

[[gnu::const]]
static WindowStyle
GetDialogStyle() noexcept
{
  WindowStyle style;
  style.Hide();
  style.ControlParent();
  return style;
}

WidgetDialog::WidgetDialog(const DialogLook &look)
  :WndForm(look),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow())
{
}

WidgetDialog::WidgetDialog(SingleWindow &parent, const DialogLook &look,
                           const PixelRect &rc, const TCHAR *caption,
                           Widget *_widget) noexcept
  :WndForm(parent, look, rc, caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   full(false), auto_size(false)
{
  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout());
}

WidgetDialog::WidgetDialog(Auto, SingleWindow &parent, const DialogLook &look,
                           const TCHAR *caption) noexcept
  :WndForm(parent, look, parent.GetClientRect(), caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   full(false), auto_size(true)
{
}

WidgetDialog::WidgetDialog(Auto tag, SingleWindow &parent, const DialogLook &look,
                           const TCHAR *caption,
                           Widget *_widget) noexcept
  :WidgetDialog(tag, parent, look, caption)
{
  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout());
}

WidgetDialog::WidgetDialog(Full, SingleWindow &parent, const DialogLook &look,
                           const TCHAR *caption) noexcept
  :WndForm(parent, look, parent.GetClientRect(), caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   full(true), auto_size(false)
{
}

WidgetDialog::WidgetDialog(Full tag, SingleWindow &parent, const DialogLook &look,
                           const TCHAR *caption,
                           Widget *_widget) noexcept
  :WidgetDialog(tag, parent, look, caption)
{
  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout());
}

WidgetDialog::~WidgetDialog()
{
  /* we must override ~Window(), because in ~Window(), our own
     OnDestroy() method won't be called (during object destruction,
     this object loses its identity) */
  Destroy();
}

void
WidgetDialog::FinishPreliminary(Widget *_widget)
{
  assert(IsDefined());
  assert(!widget.IsDefined());
  assert(_widget != nullptr);

  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout());

  if (auto_size)
    AutoSize();
}

void
WidgetDialog::FinishPreliminary(std::unique_ptr<Widget> _widget) noexcept
{
  FinishPreliminary(_widget.release());
}

void
WidgetDialog::AutoSize()
{
  const PixelRect parent_rc = GetParentClientRect();
  const PixelSize parent_size = parent_rc.GetSize();

  PrepareWidget();

  // Calculate the minimum size of the dialog
  const auto min_size = ClientAreaToDialogSize(widget.Get()->GetMinimumSize());

  // Calculate the maximum size of the dialog
  const auto max_size = ClientAreaToDialogSize(widget.Get()->GetMaximumSize());

  // Calculate sizes with one button row at the bottom
  const unsigned min_height_with_buttons =
    min_size.height + Layout::GetMaximumControlHeight();
  const unsigned max_height_with_buttons =
    max_size.height + Layout::GetMaximumControlHeight();

  if (/* need full dialog height even for minimum widget height? */
      min_height_with_buttons >= parent_size.height ||
      /* try to avoid putting buttons left on portrait screens; try to
         comply with maximum widget height only on landscape
         screens */
      (parent_size.width > parent_size.height &&
       max_height_with_buttons >= parent_size.height)) {
    /* need full height, buttons must be left */
    PixelRect rc = parent_rc;
    if (max_size.height < parent_size.height)
      rc.bottom = rc.top + max_size.height;

    PixelRect remaining = buttons.LeftLayout(rc);
    PixelSize remaining_size = remaining.GetSize();
    if (remaining_size.width > max_size.width)
      rc.right -= remaining_size.width - max_size.width;

    Resize(rc.GetSize());
    widget.Move(buttons.LeftLayout());

    MoveToCenter();
    return;
  }

  /* see if buttons fit at the bottom */

  PixelRect rc = parent_rc;
  if (max_size.width < parent_size.width)
    rc.right = rc.left + max_size.width;

  PixelRect remaining = buttons.BottomLayout(rc);
  PixelSize remaining_size = remaining.GetSize();

  if (remaining_size.height > max_size.height)
    rc.bottom -= remaining_size.height - max_size.height;

  Resize(rc.GetSize());
  widget.Move(buttons.BottomLayout());

  MoveToCenter();
}

int
WidgetDialog::ShowModal()
{
  if (auto_size)
    AutoSize();
  else
    widget.Move(buttons.UpdateLayout());

  widget.Show();
  int result = WndForm::ShowModal();
  widget.Hide();
  return result;
}

void
WidgetDialog::SetModalResult(int id) noexcept
{
  if (id == mrOK) {
    if (!widget.Get()->Save(changed))
      return;
  }

  WndForm::SetModalResult(id);
}

void
WidgetDialog::OnDestroy() noexcept
{
  widget.Unprepare();

  WndForm::OnDestroy();
}

void
WidgetDialog::OnResize(PixelSize new_size) noexcept
{
  WndForm::OnResize(new_size);

  if (auto_size)
    return;

  widget.Move(buttons.UpdateLayout());
}

void
WidgetDialog::ReinitialiseLayout(const PixelRect &parent_rc) noexcept
{
  if (full)
    /* make it full-screen again on the resized main window */
    Move(parent_rc);
  else
    WndForm::ReinitialiseLayout(parent_rc);
}

void
WidgetDialog::SetDefaultFocus() noexcept
{
  if (!widget.SetFocus())
    WndForm::SetDefaultFocus();
}

bool
WidgetDialog::OnAnyKeyDown(unsigned key_code) noexcept
{
  return widget.KeyPress(key_code) ||
    buttons.KeyPress(key_code) ||
    WndForm::OnAnyKeyDown(key_code);
}

bool
DefaultWidgetDialog(SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, const PixelRect &rc, Widget &widget)
{
  WidgetDialog dialog(parent, look, rc, caption, &widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}

bool
DefaultWidgetDialog(SingleWindow &parent, const DialogLook &look,
                    const TCHAR *caption, Widget &widget)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, caption, &widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}
