/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Form/TabBar.hpp"
#include "Form/TabDisplay.hpp"
#include "Screen/PaintWindow.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Canvas.hpp"
#include "Asset.hpp"

#include <assert.h>
#include <winuser.h>

TabBarControl::TabBarControl(ContainerWindow &_parent, const DialogLook &look,
                             PixelScalar x, PixelScalar y,
                             UPixelScalar _width, UPixelScalar _height,
                             const WindowStyle style, bool _flipOrientation,
                             bool _clientOverlapTabs)
  :tab_display(NULL),
   tab_line_height((Layout::landscape ^ _flipOrientation)
                 ? (Layout::Scale(TabLineHeightInitUnscaled) * 0.75)
                 : Layout::Scale(TabLineHeightInitUnscaled)),
   flip_orientation(_flipOrientation),
   client_overlap_tabs(_clientOverlapTabs),
   page_flipped_callback(NULL)
{
  set(_parent, 0, 0, _parent.get_width(), _parent.get_height(), style),

  tab_display = new TabDisplay(*this, look, *this,
                                 x, y, _width, _height,
                                 flip_orientation);

  PixelRect rc = get_client_rect();
  if (!_clientOverlapTabs) {
    if (Layout::landscape ^ flip_orientation)
      rc.left += tab_display->GetTabWidth();
    else
      rc.top += tab_display->GetTabHeight();
  }

  pager.Move(rc);
}

TabBarControl::~TabBarControl()
{
  delete tab_display;

  for (auto i = buttons.begin(), end = buttons.end(); i != end; ++i)
    delete *i;

  reset();
}

void
TabBarControl::SetClientOverlapTabs(bool value)
{
  if (client_overlap_tabs == value)
    return;

  client_overlap_tabs = value;

  PixelRect rc = get_client_rect();
  if (!client_overlap_tabs) {
    if (Layout::landscape ^ flip_orientation)
      rc.left += tab_display->GetTabWidth();
    else
      rc.top += tab_display->GetTabHeight();
  }

  pager.Move(rc);
}

const TCHAR*
TabBarControl::GetButtonCaption(unsigned i) const
{
  assert(i < buttons.size());

  return buttons[i]->caption.c_str();
}

unsigned
TabBarControl::AddTab(Widget *widget, const TCHAR *caption,
                      bool button_only, const Bitmap *bmp)
{
  pager.Add(widget);

  OneTabButton *b = new OneTabButton(caption, button_only, bmp);
  buttons.append(b);

  return buttons.size() - 1;
}

void
TabBarControl::ClickPage(unsigned i)
{
  assert(i < buttons.size());

  pager.ClickPage(i);

  if (tab_display != NULL)
    tab_display->invalidate();
}

void
TabBarControl::SetCurrentPage(unsigned i)
{
  assert(i < buttons.size());

  const unsigned old_current = pager.GetCurrentIndex();

  bool success = pager.SetCurrent(i);

  if (success && old_current != pager.GetCurrentIndex() &&
      page_flipped_callback != NULL)
    page_flipped_callback();

  if (tab_display != NULL)
    tab_display->invalidate();
}

void
TabBarControl::NextPage()
{
  if (pager.Next(HasPointer()) && page_flipped_callback != NULL)
    page_flipped_callback();
}

void
TabBarControl::PreviousPage()
{
  if (pager.Previous(HasPointer()) && page_flipped_callback != NULL)
    page_flipped_callback();
}

const PixelRect &
TabBarControl::GetButtonSize(unsigned i) const
{
  const static PixelRect rcFallback = {0, 0, 0, 0};

  if (i >= buttons.size())
    return rcFallback; // should never be used

  if (buttons[i]->but_size.left < buttons[i]->but_size.right)
    return buttons[i]->but_size;

  const UPixelScalar margin = 1;

  /*
  bool partialTab = false;
  if ( ((Layout::landscape ^ flip_orientation) && tab_display->GetTabHeight() < get_height()) ||
      ((!Layout::landscape ^ flip_orientation) && tab_display->GetTabWidth() < get_width()) )
    partialTab = true;
  */

  const UPixelScalar finalmargin = 1; //partialTab ? tab_line_height - 1 * margin : margin;
  // Todo make the final margin display on either beginning or end of tab bar
  // depending on position of tab bar

  PixelRect rc;

  if (Layout::landscape ^ flip_orientation) {
    const UPixelScalar but_height =
       (tab_display->GetTabHeight() - finalmargin) / buttons.size() - margin;

    rc.left = 0;
    rc.right = tab_display->GetTabWidth() - tab_line_height;

    rc.top = finalmargin + (margin + but_height) * i;
    rc.bottom = rc.top + but_height;

  } else {
    const unsigned portraitRows = (buttons.size() > 4) ? 2 : 1;

    const unsigned portraitColumnsRow0 = ((portraitRows == 1)
       ? buttons.size() : buttons.size() / 2);
    const unsigned portraitColumnsRow1 = ((portraitRows == 1)
       ? 0 : buttons.size() - buttons.size() / 2);

    const unsigned row = (i > (portraitColumnsRow0 - 1)) ? 1 : 0;

    const UPixelScalar rowheight = (tab_display->GetTabHeight() - tab_line_height)
        / portraitRows - margin;

    const UPixelScalar but_width =
          (tab_display->GetTabWidth() - finalmargin) /
          ((row == 0) ? portraitColumnsRow0 : portraitColumnsRow1) - margin;

    rc.top = row * (rowheight + margin);
    rc.bottom = rc.top + rowheight;

    rc.left = finalmargin + (margin + but_width) * (row ? (i - portraitColumnsRow0) : i);
    rc.right = rc.left + but_width;
  }

  buttons[i]->but_size = rc;
  return buttons[i]->but_size;
}

UPixelScalar
TabBarControl::GetTabHeight() const
{
  return tab_display->GetTabHeight();
}

UPixelScalar
TabBarControl::GetTabWidth() const
{
  return tab_display->GetTabWidth();
}

void
TabBarControl::on_create()
{
  ContainerWindow::on_create();

  const PixelRect rc = get_client_rect();
  pager.Initialise(*this, rc);
  pager.Prepare(*this, rc);
  pager.Show(rc);
}

void
TabBarControl::on_destroy()
{
  pager.Hide();
  pager.Unprepare();

  ContainerWindow::on_destroy();
}
