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

#include "Form/GridView.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Window.hpp"

#include <assert.h>

GridView::GridView(ContainerWindow &parent,
                   PixelScalar x, PixelScalar y,
                   UPixelScalar width, UPixelScalar height,
                   const DialogLook &_look,
                   const WindowStyle style)
 :look(_look)
{
  set(parent, x, y, width, height, style);
  columnWidth = Layout::Scale(78);
  rowHeight = Layout::Scale(42);
  horizontalSpacing = Layout::Scale(0);
  verticalSpacing = Layout::Scale(0);
  const PixelRect rc = get_client_rect();
  numColumns = (rc.right - rc.left + horizontalSpacing)
    / (columnWidth + horizontalSpacing);
  if (numColumns == 0)
    numColumns = 1;
  numRows = (rc.bottom - rc.top + verticalSpacing)
    / (rowHeight + verticalSpacing);
  if (numRows == 0)
    numRows = 1;
  currentPage = 0;
}

void
GridView::SetItems(const TrivialArray<Window *, MAX_ITEMS> &items)
{
  (TrivialArray<Window *, MAX_ITEMS> &)this->items = items;
  RefreshLayout();
}

UPixelScalar
GridView::GetColumnWidth() const
{
  return columnWidth;
}

UPixelScalar
GridView::GetRowHeight() const
{
  return rowHeight;
}

unsigned
GridView::GetCurrentPage() const
{
  return currentPage;
}

unsigned
GridView::GetNumColumns() const
{
  return numColumns;
}

unsigned
GridView::GetNumRows() const
{
  return numRows;
}

void
GridView::SetNumRows(unsigned _numRows)
{
  numRows = _numRows;
  RefreshLayout();
}

void
GridView::RefreshLayout()
{
  const PixelRect rc = get_client_rect();
  unsigned maxColumns = (rc.right - rc.left + horizontalSpacing)
    / (columnWidth + horizontalSpacing);
  if (maxColumns == 0)
    maxColumns = 1;

  unsigned maxRows = (rc.bottom - rc.top + verticalSpacing)
    / (rowHeight + verticalSpacing);
  if (maxRows == 0)
    maxRows = 1;

  if (maxColumns < numColumns)
     numColumns = maxColumns;

  if (maxRows < numRows)
    numRows = maxRows;

  unsigned pageSize = numColumns * numRows;

  // Center grid in the client area
  unsigned reminderH = rc.right - rc.left + horizontalSpacing
    - numColumns * (columnWidth + horizontalSpacing);
  unsigned reminderV = rc.bottom - rc.top + verticalSpacing
    - numRows * (rowHeight + verticalSpacing);
  unsigned leftOrigin = rc.left + reminderH / 2;
  unsigned topOrigin= rc.top + reminderV / 2;

  // Determine current page from item that has focus
  // If there is no item with focus or the item with focus is on current page,
  // the current page remains unchanged
  signed focusPos = GetIndexOfItemInFocus();
  if (focusPos != -1)
    currentPage = focusPos / pageSize;

  for (unsigned i = items.size(); i--;) {
    unsigned pagePos = i % pageSize;
    unsigned itemPage = i / pageSize;
    unsigned colNum = pagePos % numColumns;
    unsigned rowNum = pagePos / numColumns;

    items[i]->move(leftOrigin + colNum * (columnWidth + horizontalSpacing),
                   topOrigin + rowNum * (rowHeight + verticalSpacing),
                   columnWidth, rowHeight);
    items[i]->set_visible(itemPage == currentPage);
  }
}

/*
 * -1 means that there is no window in focus
 */
signed
GridView::GetIndexOfItemInFocus()
{
  signed index = -1;
  for (unsigned i = items.size(); i--;) {
    if (items[i]->has_focus()) {
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
  unsigned pageSize = numColumns * numRows;
  unsigned lastPage = items.size() / pageSize;

  if (direction == Direction::LEFT && currentPage > 0)
    currentPage--;
  else if (direction == Direction::RIGHT && currentPage < lastPage)
    currentPage++;
  else
    return;

  unsigned currentPageSize = currentPage == lastPage
    ? items.size() % pageSize
    : pageSize;

  signed focusPos = GetIndexOfItemInFocus();

  if (focusPos != -1) {
    unsigned oldPagePos = focusPos % pageSize;
    unsigned oldRowNum = oldPagePos / numColumns;

    if (direction == Direction::LEFT)
      // last column in the same row
      newPos = currentPage * pageSize + (oldRowNum + 1) * numColumns - 1;
    else { // direction == Direction::RIGHT
      // first column in the same row
      newPos = currentPage * pageSize + oldRowNum * numColumns;

      if (newPos >= (signed)items.size()) {
        // first columns in the last row
        newPos = currentPage * pageSize + currentPageSize
          - (currentPageSize - 1) % numColumns - 1;
      }
    }
  }

  if (newPos != -1) {
    items[newPos]->set_focus();
    /* unable to set the focus on the desired item, let's try
       Tab/Shift-Tab behavior instead */
    if (!items[newPos]->has_focus()) {
      if (direction == Direction::LEFT)
        newPos = GetNextEnabledItemIndex(currentPage * pageSize
                                         + currentPageSize, direction);
      else
        newPos = GetNextEnabledItemIndex(currentPage * pageSize - 1,
                                         direction);

      // set focus only if it is on the same page
      if (newPos != -1 &&
          newPos >= (signed)(currentPage * pageSize) &&
          newPos < (signed)(currentPage * pageSize + currentPageSize)) {
        items[newPos]->set_focus();
      }
    } else if (focusPos != -1) {
#ifdef USE_GDI
      HWND oldFocusHwnd = ::GetFocus();
      if (oldFocusHwnd != NULL)
        ::SendMessage(oldFocusHwnd, WM_CANCELMODE, 0, 0);
#else
      items[focusPos]->ClearFocus();
#endif /* USE_GDI */
    }

    RefreshLayout();
  }
}

signed
GridView::GetNextEnabledItemIndex(signed currIndex, Direction direction)
{
  signed nextPos = -1;
  if (direction == Direction::UP || direction == Direction::LEFT) {
    // Treat as Shift-Tab
    for (signed i = currIndex - 1; i >= 0; i--)
      if (items[i]->is_enabled())
        return i;
  } else {
    // Treat as Tab
    for (unsigned i = currIndex + 1; i < items.size(); i++)
      if (items[i]->is_enabled())
        return i;
  }

  return nextPos;
}

/**
 * return -1 means the current position cannot be changed without
 * going to a different page
 */
signed
GridView::GetNextItemIndex(unsigned currIndex, Direction direction)
{
  unsigned pageSize = numColumns * numRows;
  unsigned pagePos = currIndex % pageSize;
  unsigned colNum = pagePos % numColumns;
  unsigned rowNum = pagePos / numColumns;
  signed nextPos = -1;

  switch(direction) {
  case Direction::LEFT:
    if (colNum > 0)
      nextPos = currIndex - 1;
    break;

  case Direction::RIGHT:
    if (colNum + 1 < numColumns && currIndex + 1 < items.size())
      nextPos = currIndex + 1;
    break;

  case Direction::UP:
    if (rowNum > 0)
      nextPos = currIndex - numColumns;
    break;

  case Direction::DOWN:
    if (rowNum + 1 < numRows && currIndex + numColumns < items.size())
      nextPos = currIndex + numColumns;
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
      items[newFocusPos]->set_focus();
      if (!items[newFocusPos]->has_focus()) {
        /* unable to set the focus, let's try simple Tab/Shift-Tab
           behavior instead */
        newFocusPos = GetNextEnabledItemIndex(focusPos, direction);
        if (newFocusPos != -1)
          items[newFocusPos]->set_focus();
      }
      RefreshLayout();
    } else
      ShowNextPage(direction);
  }
}

#ifdef USE_GDI
void
GridView::on_paint(Canvas &canvas)
{
  canvas.clear(look.background_color);
}
#endif

