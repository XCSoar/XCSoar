/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Asset.hpp"

#ifdef HAVE_CLIPPING
#include "Screen/Canvas.hpp"
#include "Look/DialogLook.hpp"
#endif

gcc_const
static PixelRect
MakePagerRect(PixelRect rc, const PixelRect &tab_rc, bool vertical)
{
  if (vertical)
    rc.left = tab_rc.right;
  else
    rc.top = tab_rc.bottom;
  return rc;
}

TabBarControl::TabBarControl(ContainerWindow &_parent, const DialogLook &look,
                             PixelRect tab_rc,
                             const WindowStyle style, bool vertical)
  :tab_display(nullptr)
{
  Create(_parent, _parent.GetClientRect(), style);

  tab_display = new TabDisplay(*this, look, *this, tab_rc, vertical);

  pager.Move(MakePagerRect(GetClientRect(), tab_rc, vertical));
}

TabBarControl::~TabBarControl()
{
  delete tab_display;

  Destroy();
}

PixelSize
TabBarControl::GetMinimumSize() const
{
  PixelSize s = pager.GetMinimumSize();
  if (tab_display != nullptr) {
    if (tab_display->IsVertical())
      s.cx += tab_display->GetWidth();
    else
      s.cy += tab_display->GetHeight();
  }

  return s;
}

PixelSize
TabBarControl::GetMaximumSize() const
{
  PixelSize s = pager.GetMaximumSize();
  if (tab_display != nullptr) {
    if (tab_display->IsVertical())
      s.cx += tab_display->GetWidth();
    else
      s.cy += tab_display->GetHeight();
  }

  return s;
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

  if (tab_display != nullptr)
    tab_display->Invalidate();

  if (page_flipped_callback)
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

  if (tab_display != nullptr)
    tab_display->Invalidate();

  if (page_flipped_callback)
    page_flipped_callback();
}

void
TabBarControl::NextPage()
{
  if (!pager.Next(HasPointer()))
    /* failed to switch */
    return;

  if (tab_display != nullptr)
    tab_display->Invalidate();

  if (page_flipped_callback)
    page_flipped_callback();
}

void
TabBarControl::PreviousPage()
{
  if (!pager.Previous(HasPointer()))
    /* failed to switch */
    return;

  if (tab_display != nullptr)
    tab_display->Invalidate();

  if (page_flipped_callback)
    page_flipped_callback();
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

void
TabBarControl::OnResize(PixelSize new_size)
{
  ContainerWindow::OnResize(new_size);

  if (tab_display != nullptr) {
    const PixelRect tab_rc = tab_display->GetPosition();
    PixelRect rc = GetClientRect();
    if (tab_display->IsVertical())
      rc.left = tab_rc.right;
    else
      rc.top = tab_rc.bottom;

    pager.Move(rc);
  }
}

#ifdef HAVE_CLIPPING

void
TabBarControl::OnPaint(Canvas &canvas)
{
  /* erase the remaining background area, just in case the TabDisplay
     does not cover the whole height or width; this is necessary only
     on GDI */

  if (tab_display != nullptr)
    canvas.Clear(tab_display->GetLook().background_color);
}

#endif
