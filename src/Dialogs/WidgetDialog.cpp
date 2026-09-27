// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/WidgetDialog.hpp"
#include "Look/DialogLook.hpp"
#include "Form/Form.hpp"
#include "Form/ButtonPanel.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Widget/Widget.hpp"
#include "Language/Language.hpp"
#include "Renderer/BoxShadowRenderer.hpp"
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
                           const PixelRect &rc, const char *caption,
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
                           const char *caption) noexcept
  :WndForm(parent, look, parent.GetClientRect(), caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   full(false), auto_size(true)
{
}

WidgetDialog::WidgetDialog(Auto tag, SingleWindow &parent, const DialogLook &look,
                           const char *caption,
                           Widget *_widget) noexcept
  :WidgetDialog(tag, parent, look, caption)
{
  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout());
}

WidgetDialog::WidgetDialog(Full, SingleWindow &parent, const DialogLook &look,
                           const char *caption) noexcept
  :WndForm(parent, look, parent.GetClientRect(), caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   full(true), auto_size(false)
{
}

WidgetDialog::WidgetDialog(Full tag, SingleWindow &parent, const DialogLook &look,
                           const char *caption,
                           Widget *_widget) noexcept
  :WidgetDialog(tag, parent, look, caption)
{
  widget.Set(_widget);
  widget.Move(buttons.UpdateLayout());
}

WidgetDialog::WidgetDialog(Floating, SingleWindow &parent,
                           const DialogLook &look,
                           const char *caption,
                           GroupedListWidget *list) noexcept
  :WndForm(parent, look,
           PixelRect{Layout::Scale(PixelSize{220u, 220u})},
           caption, GetDialogStyle()),
   buttons(GetClientAreaWindow(), look.button),
   widget(GetClientAreaWindow()),
   full(false), auto_size(false)
{
  widget.Set(list);
  widget.Move(LayoutButtons());
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

static unsigned
OuterLimit(unsigned parent, unsigned inset) noexcept
{
  if (parent <= inset * 2)
    return parent > 1 ? parent - 1 : parent;

  return parent - 2 * inset;
}

PixelRect
WidgetDialog::LayoutButtons() noexcept
{
  /* a list this dialog was fitted to is short, so the client is
     wider than it is tall.  The strip stays under the list. */
  if (fit_client_width > 0)
    return buttons.BottomLayout();

  return buttons.UpdateLayout();
}

void
WidgetDialog::FitToList(const PixelRect &parent_rc,
                        unsigned preferred_client_width) noexcept
{
  if (fitting)
    return;

  fit_client_width = preferred_client_width;
  fitting = true;

  auto &list = static_cast<GroupedListWidget &>(GetWidget());

  const unsigned parent_w = parent_rc.GetWidth();
  const unsigned parent_h = parent_rc.GetHeight();
  unsigned inset = BoxShadowExtent(BoxShadowStyle::DIALOG);
  if (inset < 1)
    inset = 1;

  const unsigned max_w = OuterLimit(parent_w, inset);
  const unsigned max_h = OuterLimit(parent_h, inset);
  const unsigned frame_w = ClientAreaToDialogSize({}).width;
  const unsigned frame_h = ClientAreaToDialogSize({}).height;

  unsigned client_w = preferred_client_width;
  if (client_w + frame_w > max_w)
    client_w = max_w > frame_w ? max_w - frame_w : max_w;

  unsigned client_h = max_h > frame_h ? max_h - frame_h : 1;
  const unsigned min_client_h = Layout::GetMaximumControlHeight();

  for (unsigned pass = 0; pass < 2; ++pass) {
    const PixelSize outer = ClientAreaToDialogSize({client_w, client_h});
    if (GetSize() != outer)
      Resize(outer);

    const PixelRect widget_rc = LayoutButtons();
    list.Move(widget_rc);
    list.UpdateLayout();

    unsigned content = list.GetContentHeight();
    if (content == 0)
      content = list.GetMinimumSize().height;

    const unsigned widget_h = widget_rc.GetHeight();
    if (widget_h > content && client_h > content) {
      const unsigned spare = widget_h - content;
      client_h = client_h > spare ? client_h - spare : content;
    }

    if (client_h < min_client_h)
      client_h = min_client_h;

    if (client_h + frame_h > max_h)
      client_h = max_h > frame_h ? max_h - frame_h : client_h;
  }

  const PixelSize size = GetSize();
  int x = (int)parent_rc.left +
    ((int)parent_w - (int)size.width) / 2;
  int y = (int)parent_rc.top +
    ((int)parent_h - (int)size.height) / 2;
  if (x < (int)parent_rc.left)
    x = parent_rc.left;
  if (y < (int)parent_rc.top)
    y = parent_rc.top;

  Move(PixelPoint{x, y});

  fitting = false;
}

void
WidgetDialog::RefitList() noexcept
{
  if (fitting)
    return;

  auto &list = static_cast<GroupedListWidget &>(GetWidget());
  const unsigned text = list.PreferredTextWidth();
  if (text == 0)
    return;

  const unsigned width = text + 2 * Layout::VptScale(10) +
    2 * Layout::GetTextPadding();
  FitToList(GetParentClientRect(), width);
}

void
WidgetDialog::PrepareFloatingList() noexcept
{
  EnableCursorSelection();
  ResyncButtonPanelSelection();

  auto &list = static_cast<GroupedListWidget &>(GetWidget());
  list.SetActionBar(buttons);
  list.SetCursorCallback([this](int){ RefitList(); });
  PrepareWidget();
}

int
WidgetDialog::ShowModal()
{
  if (auto_size)
    AutoSize();
  else
    widget.Move(LayoutButtons());

  widget.Show();
  if (!auto_size) {
    /* Ensure button layout is recalculated with any metrics that may have
       become available when the widget was shown (fixes caption clipping on
       some scaled/font configurations).  Keep this non-auto dialogs only,
       so AutoSize()'s LeftLayout()/BottomLayout() decision remains intact. */
    widget.Move(LayoutButtons());
  }
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

  widget.Move(LayoutButtons());
}

void
WidgetDialog::ReinitialiseLayout(const PixelRect &parent_rc) noexcept
{
  if (fit_client_width > 0)
    FitToList(parent_rc, fit_client_width);
  else if (full)
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
                    const char *caption, const PixelRect &rc, Widget &widget)
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
                    const char *caption, Widget &widget)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, parent, look, caption, &widget);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();

  /* the caller manages the Widget */
  dialog.StealWidget();

  return dialog.GetChanged();
}
