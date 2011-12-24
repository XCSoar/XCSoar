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

#include "Form/Tabbed.hpp"
#include "WindowWidget.hpp"

#include <assert.h>

TabbedControl::TabbedControl(ContainerWindow &parent,
                             PixelScalar x, PixelScalar y,
                             UPixelScalar width, UPixelScalar height,
                             const WindowStyle style)
  :page_flipped_callback(NULL)
{
  set(parent, x, y, width, height, style);
}

void
TabbedControl::AddPage(Widget *w)
{
  assert(defined());

  if (tabs.empty()) {
    current = 0;
  } else {
    assert(current < tabs.size());
  }

  const bool show = tabs.empty();
  tabs.append(w);
  Page &tab = tabs.back();

  const PixelRect rc = get_client_rect();
  w->Initialise(*this, rc);

  if (show) {
    tab.prepared = true;
    w->Prepare(*this, rc);
    w->Show(rc);
  }
}

void
TabbedControl::AddClient(Window *w)
{
  /* backwards compatibility: make sure the Window is hidden */
  w->hide();

  AddPage(new WindowWidget(w));
}

bool
TabbedControl::SetCurrentPage(unsigned i, bool click)
{
  assert(i < tabs.size());

  if (i == current) {
    if (!click) {
      return true;
    } else {
      tabs[i].widget->ReClick();
      return true;
    }
  }

  assert(tabs[current].prepared);
  if (!tabs[current].widget->Leave())
    return false;

  if (click && !tabs[i].widget->Click())
    return false;

  tabs[current].widget->Hide();

  current = i;

  if (!tabs[current].prepared) {
    tabs[current].prepared = true;
    tabs[current].widget->Prepare(*this, get_client_rect());
  }

  tabs[current].widget->Show(get_client_rect());

  if (page_flipped_callback != NULL)
    page_flipped_callback();

  return true;
}

void
TabbedControl::NextPage()
{
  if (tabs.size() < 2)
    return;

  assert(current < tabs.size());

  SetCurrentPage((current + 1) % tabs.size());
}

void
TabbedControl::PreviousPage()
{
  if (tabs.size() < 2)
    return;

  assert(current < tabs.size());

  SetCurrentPage((current + tabs.size() - 1) % tabs.size());
}

bool
TabbedControl::ClickPage(unsigned i)
{
  assert(i < tabs.size());

  return SetCurrentPage(i, true);
}

bool
TabbedControl::Save(bool &changed, bool &require_restart)
{
  for (auto i = tabs.begin(), end = tabs.end(); i != end; ++i)
    if (i->prepared && !i->widget->Save(changed, require_restart))
      return false;

  return true;
}

void
TabbedControl::on_resize(UPixelScalar width, UPixelScalar height)
{
  ContainerWindow::on_resize(width, height);

  if (!tabs.empty()) {
    /* adjust the current page: hide and show it again with the new
       dimensions */
    Page &tab = tabs[current];
    assert(tab.prepared);
    tab.widget->Move(get_client_rect());
  }
}

void
TabbedControl::on_destroy()
{
  if (!tabs.empty()) {
    assert(tabs[current].prepared);
    tabs[current].widget->Leave();
    tabs[current].widget->Hide();
  }

  for (auto i = tabs.begin(), end = tabs.end(); i != end; ++i) {
    if (i->prepared)
      i->widget->Unprepare();

    delete i->widget;
  }

  tabs.clear();

  ContainerWindow::on_destroy();
}
