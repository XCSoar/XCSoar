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

#include "Form/GridView.hpp"

void
GridView::Create(ContainerWindow &parent, const DialogLook &look,
                 const PixelRect &rc, const WindowStyle style,
                 unsigned _column_width, unsigned _row_height)
{
  column_width = _column_width;
  row_height = _row_height;
  PanelControl::Create(parent, look, rc, style);
  current_page = 0;
}

void
GridView::RefreshLayout()
{
  const PixelRect rc = GetClientRect();
  const unsigned width = rc.GetWidth(), height = rc.GetHeight();

  constexpr unsigned horizontal_spacing = 0;
  constexpr unsigned vertical_spacing = 0;

  num_columns = (width + horizontal_spacing)
    / (column_width + horizontal_spacing);
  if (num_columns == 0)
    num_columns = 1;
  num_rows = (height + vertical_spacing)
    / (row_height + vertical_spacing);
  if (num_rows == 0)
    num_rows = 1;

  unsigned pageSize = num_columns * num_rows;

  // Center grid in the client area
  unsigned reminderH = width + horizontal_spacing
    - num_columns * (column_width + horizontal_spacing);
  unsigned reminderV = height + vertical_spacing
    - num_rows * (row_height + vertical_spacing);
  unsigned leftOrigin = rc.left + reminderH / 2;
  unsigned topOrigin= rc.top + reminderV / 2;

  // Determine current page from item that has focus
  // If there is no item with focus or the item with focus is on current page,
  // the current page remains unchanged
  signed focusPos = GetIndexOfItemInFocus();
  if (focusPos != -1)
    current_page = focusPos / pageSize;

  for (unsigned i = items.size(); i--;) {
    unsigned pagePos = i % pageSize;
    unsigned itemPage = i / pageSize;
    unsigned colNum = pagePos % num_columns;
    unsigned rowNum = pagePos / num_columns;

    items[i]->Move(leftOrigin + colNum * (column_width + horizontal_spacing),
                   topOrigin + rowNum * (row_height + vertical_spacing),
                   column_width, row_height);
    items[i]->SetVisible(itemPage == current_page);
  }
}

/*
 * -1 means that there is no window in focus
 */
signed
GridView::GetIndexOfItemInFocus() const
{
  signed index = -1;
  for (unsigned i = items.size(); i--;) {
    if (items[i]->HasFocus()) {
      index = i;
      break;
    }
  }
  return index;
}


void
GridView::ShowNextPage(Direction direction)
{
  signed newPos = -1;
  unsigned pageSize = num_columns * num_rows;
  unsigned lastPage = items.size() / pageSize;

  if (direction == Direction::LEFT && current_page > 0)
    current_page--;
  else if (direction == Direction::RIGHT && current_page < lastPage)
    current_page++;
  else
    return;

  unsigned currentPageSize = current_page == lastPage
    ? items.size() % pageSize
    : pageSize;

  signed focusPos = GetIndexOfItemInFocus();

  if (focusPos != -1) {
    unsigned oldPagePos = focusPos % pageSize;
    unsigned oldRowNum = oldPagePos / num_columns;

    if (direction == Direction::LEFT)
      // last column in the same row
      newPos = current_page * pageSize + (oldRowNum + 1) * num_columns - 1;
    else { // direction == Direction::RIGHT
      // first column in the same row
      newPos = current_page * pageSize + oldRowNum * num_columns;

      if (newPos >= (signed)items.size()) {
        // first columns in the last row
        newPos = current_page * pageSize + currentPageSize
          - (currentPageSize - 1) % num_columns - 1;
      }
    }
  }

  if (newPos != -1) {
    items[newPos]->SetFocus();
    /* unable to set the focus on the desired item, let's try
       Tab/Shift-Tab behavior instead */
    if (!items[newPos]->HasFocus()) {
      if (direction == Direction::LEFT)
        newPos = GetNextEnabledItemIndex(current_page * pageSize
                                         + currentPageSize, direction);
      else
        newPos = GetNextEnabledItemIndex(current_page * pageSize - 1,
                                         direction);

      // set focus only if it is on the same page
      if (newPos != -1 &&
          newPos >= (signed)(current_page * pageSize) &&
          newPos < (signed)(current_page * pageSize + currentPageSize)) {
        items[newPos]->SetFocus();
      }
    } else if (focusPos != -1) {
#ifdef USE_WINUSER
      HWND oldFocusHwnd = ::GetFocus();
      if (oldFocusHwnd != nullptr)
        ::SendMessage(oldFocusHwnd, WM_CANCELMODE, 0, 0);
#else
      items[focusPos]->ClearFocus();
#endif /* USE_WINUSER */
    }

    RefreshLayout();
  }
}

signed
GridView::GetNextEnabledItemIndex(signed currIndex, Direction direction) const
{
  signed nextPos = -1;
  if (direction == Direction::UP || direction == Direction::LEFT) {
    // Treat as Shift-Tab
    for (signed i = currIndex - 1; i >= 0; i--)
      if (items[i]->IsEnabled())
        return i;
  } else {
    // Treat as Tab
    for (unsigned i = currIndex + 1; i < items.size(); i++)
      if (items[i]->IsEnabled())
        return i;
  }

  return nextPos;
}

/**
 * return -1 means the current position cannot be changed without
 * going to a different page
 */
signed
GridView::GetNextItemIndex(unsigned currIndex, Direction direction) const
{
  unsigned pageSize = num_columns * num_rows;
  unsigned pagePos = currIndex % pageSize;
  unsigned colNum = pagePos % num_columns;
  unsigned rowNum = pagePos / num_columns;
  signed nextPos = -1;

  switch(direction) {
  case Direction::LEFT:
    if (colNum > 0)
      nextPos = currIndex - 1;
    break;

  case Direction::RIGHT:
    if (colNum + 1 < num_columns && currIndex + 1 < items.size())
      nextPos = currIndex + 1;
    break;

  case Direction::UP:
    if (rowNum > 0)
      nextPos = currIndex - num_columns;
    break;

  case Direction::DOWN:
    if (rowNum + 1 < num_rows && currIndex + num_columns < items.size())
      nextPos = currIndex + num_columns;
    break;
  }

  return nextPos;
}

void
GridView::MoveFocus(Direction direction)
{
  signed focusPos = GetIndexOfItemInFocus();

  if (focusPos == -1)
    // We are on a page without enabled items.
    // We should show the next page if exists
    ShowNextPage(direction);
  else {
    signed newFocusPos = GetNextItemIndex(focusPos, direction);

    if (newFocusPos != -1) {
      // we are on the same page
      items[newFocusPos]->SetFocus();
      if (!items[newFocusPos]->HasFocus()) {
        /* unable to set the focus, let's try simple Tab/Shift-Tab
           behavior instead */
        newFocusPos = GetNextEnabledItemIndex(focusPos, direction);
        if (newFocusPos != -1)
          items[newFocusPos]->SetFocus();
      }
      RefreshLayout();
    } else
      ShowNextPage(direction);
  }
}

void
GridView::OnResize(PixelSize new_size)
{
  PanelControl::OnResize(new_size);
  RefreshLayout();
}
