/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "ListWidget.hpp"
#include "Screen/Window.hpp"
#include "Screen/Layout.hpp"

PixelSize
ListWidget::GetMinimumSize() const
{
  return { unsigned(Layout::Scale(200u)),
      /* a list makes only sense when the user sees more than one row
         at a time */
      2u * GetList().GetItemHeight() };
}

PixelSize
ListWidget::GetMaximumSize() const
{
  return PixelSize { 4096, 4096 };
}

ListControl &
ListWidget::CreateList(ContainerWindow &parent, const DialogLook &look,
                       const PixelRect &rc, unsigned row_height)
{
  WindowStyle list_style;
  list_style.Hide();
  list_style.TabStop();
  list_style.Border();

  ListControl *list =
    new ListControl(parent, look, rc, list_style, row_height);
  list->SetItemRenderer(this);
  list->SetCursorHandler(this);
  SetWindow(list);
  return *list;
}
