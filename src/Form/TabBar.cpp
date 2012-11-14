/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
                             PixelRect tab_rc,
                             const WindowStyle style, bool _flipOrientation)
  :tab_display(NULL),
   tab_line_height((Layout::landscape ^ _flipOrientation)
                 ? (Layout::Scale(TabLineHeightInitUnscaled) * 0.75)
                 : Layout::Scale(TabLineHeightInitUnscaled)),
   flip_orientation(_flipOrientation),
   page_flipped_callback(NULL)
{
  Create(_parent, _parent.GetClientRect(), style);

  tab_display = new TabDisplay(*this, look, *this, tab_rc, flip_orientation);

  PixelRect rc = GetClientRect();
  if (Layout::landscape ^ flip_orientation)
    rc.left += tab_display->GetTabWidth();
  else
    rc.top += tab_display->GetTabHeight();

  pager.Move(rc);
}

TabBarControl::~TabBarControl()
{
  delete tab_display;

  Destroy();
}

const TCHAR*
TabBarControl::GetButtonCaption(unsigned i) const
{
  return tab_display->GetCaption(i);
}

unsigned
TabBarControl::AddTab(Widget *widget, const TCHAR *caption, const Bitmap *bmp)
{
  pager.Add(widget);
  tab_display->Add(caption, bmp);
  return GetTabCount() - 1;
}

void
TabBarControl::ClickPage(unsigned i)
{
  const bool is_current = i == pager.GetCurrentIndex();
  if (!pager.ClickPage(i) || is_current)
    /* failure */
    return;

  /* switching to a new page by mouse click focuses the first control
     of the page, which is important for Altair hot keys */
  pager.SetFocus();

  if (tab_display != NULL)
    tab_display->Invalidate();

  if (page_flipped_callback != NULL)
    page_flipped_callback();
}

void
TabBarControl::SetCurrentPage(unsigned i)
{
  if (i == pager.GetCurrentIndex())
    /* no-op */
    return;

  if (!pager.SetCurrent(i))
    /* failed to switch */
    return;

  if (tab_display != NULL)
    tab_display->Invalidate();

  if (page_flipped_callback != NULL)
    page_flipped_callback();
}

void
TabBarControl::NextPage()
{
  if (!pager.Next(HasPointer()))
    /* failed to switch */
    return;

  if (tab_display != NULL)
    tab_display->Invalidate();

  if (page_flipped_callback != NULL)
    page_flipped_callback();
}

void
TabBarControl::PreviousPage()
{
  if (!pager.Previous(HasPointer()))
    /* failed to switch */
    return;

  if (tab_display != NULL)
    tab_display->Invalidate();

  if (page_flipped_callback != NULL)
    page_flipped_callback();
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
TabBarControl::OnCreate()
{
  ContainerWindow::OnCreate();

  const PixelRect rc = GetClientRect();
  pager.Initialise(*this, rc);
  pager.Prepare(*this, rc);
  pager.Show(rc);
}

void
TabBarControl::OnDestroy()
{
  pager.Hide();
  pager.Unprepare();

  ContainerWindow::OnDestroy();
}
