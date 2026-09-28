// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/WidgetDialog.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Renderer/BoxShadowRenderer.hpp"
#include "Screen/Layout.hpp"

/**
 * The debug tools compile WidgetDialog.cpp without the grouped
 * list.  These methods are the only ones that call it, so they
 * stay with the main program.
 */
static unsigned
OuterLimit(unsigned parent, unsigned inset) noexcept
{
  if (parent <= inset * 2)
    return parent > 1 ? parent - 1 : parent;

  return parent - 2 * inset;
}

void
WidgetDialog::RefitLayout(WidgetDialog &dialog,
                          const PixelRect &rc) noexcept
{
  dialog.FitToList(rc, dialog.fit_client_width);
}

void
WidgetDialog::FitToList(const PixelRect &parent_rc,
                        unsigned preferred_client_width) noexcept
{
  layout_refit = RefitLayout;

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

    /* a short explanation is part of the dialog.  A longer one
       scrolls in the list, so the dialog does not grow with it */
    unsigned content = list.GetFitContentHeight();
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
  list.SetSizeFollowsHelp(true);
  list.SetCursorCallback([this](int){ RefitList(); });
  PrepareWidget();
}
