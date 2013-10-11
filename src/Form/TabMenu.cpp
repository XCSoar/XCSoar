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

#include "Form/TabMenu.hpp"
#include "Form/TabMenuDisplay.hpp"
#include "Screen/Key.h"
#include "Widget/WindowWidget.hpp"

#include <assert.h>

TabMenuControl::TabMenuControl(const DialogLook &look, const TCHAR * _caption)
  :tab_display(new TabMenuDisplay(*this, look)),
   caption(_caption),
   page_flipped_callback(nullptr)
{
}

TabMenuControl::~TabMenuControl()
{
  delete tab_display;
}

void
TabMenuControl::NextPage()
{
  if (pager.Next(true))
    OnPageFlipped();
}

void
TabMenuControl::PreviousPage()
{
  if (pager.Previous(true))
    OnPageFlipped();
}

void
TabMenuControl::SetCurrentPage(unsigned page)
{
  if (pager.ClickPage(PAGE_OFFSET + page))
    OnPageFlipped();
}

bool
TabMenuControl::InvokeKeyPress(unsigned key_code)
{
  if (pager.KeyPress(key_code))
    return true;

  switch (key_code) {
  case KEY_LEFT:
#ifdef GNAV
  case '6':
#endif
    PreviousPage();
    return true;

  case KEY_RIGHT:
#ifdef GNAV
  case '7':
#endif
    NextPage();
    return true;

  default:
    return false;
  }
}

void
TabMenuControl::OnPageFlipped()
{
  const unsigned page = pager.GetCurrentIndex();
  if (page != GetMenuPage())
    SetLastContentPage(page - PAGE_OFFSET);

  if (page_flipped_callback != nullptr)
    page_flipped_callback();
}

const TCHAR *
TabMenuControl::GetPageCaption(TCHAR buffer[], size_t size) const
{
  const unsigned page = pager.GetCurrentIndex();
  if (page == GetMenuPage()) {
    return caption;
  } else {
    tab_display->FormatPageCaption(buffer, size, page - PAGE_OFFSET);
    return buffer;
  }
}

void TabMenuControl::SetLastContentPage(unsigned page)
{
  tab_display->SetCursor(page);
}

unsigned
TabMenuControl::GetLastContentPage() const
{
  return tab_display->GetCursor();
}

inline void
TabMenuControl::CreateSubMenuItem(const TabMenuPage &item)
{
  assert(item.Load != nullptr);

  Widget *widget = item.Load();
  pager.Add(widget);
}

void
TabMenuControl::InitMenu(const TabMenuPage pages_in[],
                         unsigned num_pages,
                         const TabMenuGroup groups[], unsigned n_groups)
{
  assert(pages_in);
  assert(groups != nullptr);
  assert(n_groups > 0);

  for (unsigned i = 0; i < num_pages; ++i)
    CreateSubMenuItem(pages_in[i]);

  tab_display->InitMenu(caption, pages_in, num_pages,
                        groups, n_groups);
}

void
TabMenuControl::OnCreate()
{
  ContainerWindow::OnCreate();

  const PixelRect rc = GetClientRect();
  pager.Initialise(*this, rc);
  pager.Prepare(*this, rc);
  pager.Show(rc);

  WindowStyle display_style;
  display_style.Hide();
  display_style.TabStop();
  tab_display->Create(*this, rc, display_style);

  pager.Add(new WindowWidget(tab_display));
}

void
TabMenuControl::OnDestroy()
{
  pager.Hide();
  pager.Unprepare();
  pager.Clear();

  ContainerWindow::OnDestroy();
}
