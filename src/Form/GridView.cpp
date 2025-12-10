// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Form/GridView.hpp"
#include "Math/Util.hpp"

void
GridView::Create(ContainerWindow &parent, const DialogLook &look,
                 const PixelRect &rc, const WindowStyle style,
                 unsigned _column_width, unsigned _row_height)
{
  column_width = _column_width;
  row_height = _row_height;
  PanelControl::Create(parent, look, rc, style);
  current_page = 0;
  previous_page = 0;
  saved_row_from_previous_page = 0;
  saved_row_per_page.clear();
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

  unsigned pageSize = GetPageSize();

  // Center grid in the client area
  unsigned reminderH = width + horizontal_spacing
    - num_columns * (column_width + horizontal_spacing);
  unsigned reminderV = height + vertical_spacing
    - num_rows * (row_height + vertical_spacing);
  PixelPoint top_left = rc.GetTopLeft() + PixelSize{reminderH, reminderV} / 2U;

  // Determine current page from item that has focus
  // If there is no item with focus or the item with focus is on current page,
  // the current page remains unchanged
  signed focusPos = GetIndexOfItemInFocus();
  if (focusPos != -1)
    current_page = focusPos / pageSize;

  for (unsigned i = items.size(); i--;) {
    if (!IsItemValid(i))
      continue;
    unsigned pagePos = i % pageSize;
    unsigned itemPage = i / pageSize;
    unsigned colNum = pagePos % num_columns;
    unsigned rowNum = pagePos / num_columns;

    items[i]->Move(top_left + PixelSize(colNum * (column_width + horizontal_spacing),
                                        rowNum * (row_height + vertical_spacing)),
                   PixelSize(column_width, row_height));
    items[i]->SetVisible(itemPage == current_page);
  }
}

signed
GridView::GetIndexOfItemInFocus() const
{
  for (unsigned i = 0; i < items.size(); i++) {
    if (IsItemValidAndHasFocus(i))
      return i;
  }
  return -1;
}

signed
GridView::FindEnabledInRow(unsigned pageStart, unsigned rowNum,
                            unsigned pageEnd) const noexcept
{
  unsigned rowStart = pageStart + rowNum * num_columns;
  unsigned rowEnd = std::min(rowStart + num_columns, pageEnd);
  for (unsigned i = rowStart; i < rowEnd && i < items.size(); i++) {
    if (IsItemValidAndEnabled(i))
      return i;
  }
  return -1;
}

signed
GridView::FindEnabledInColumn(unsigned pageStart, unsigned colNum,
                               unsigned pageEnd) const noexcept
{
  for (unsigned i = pageStart + colNum; i < pageEnd && i < items.size(); i += num_columns) {
    if (IsItemValidAndEnabled(i))
      return i;
  }
  return -1;
}

bool
GridView::HasEnabledInRow(unsigned pageStart, unsigned rowNum,
                          unsigned pageEnd) const noexcept
{
  return FindEnabledInRow(pageStart, rowNum, pageEnd) != -1;
}

bool
GridView::HasEnabledInDirection(unsigned focusPos, Direction direction) const noexcept
{
  PageBounds bounds = GetPageBoundsForIndex(focusPos);
  PagePosition pos = GetPagePosition(focusPos);
  unsigned pageStart = bounds.pageStart;
  unsigned pageEnd = bounds.pageEnd;
  unsigned rowNum = pos.rowNum;

  if (IsHorizontal(direction)) {
    // LEFT or RIGHT: iterate through row
    unsigned rowStart = pageStart + rowNum * num_columns;
    unsigned rowEnd = std::min(rowStart + num_columns, pageEnd);
    
    if (direction == Direction::LEFT) {
      for (unsigned i = rowStart; i < focusPos; i++) {
        if (IsItemValidAndEnabled(i))
          return true;
      }
    } else { // RIGHT
      for (unsigned i = focusPos + 1; i < rowEnd && i < pageEnd; i++) {
        if (IsItemValidAndEnabled(i))
          return true;
      }
    }
  } else {
    // UP or DOWN: iterate through column
    if (direction == Direction::UP) {
      for (signed i = focusPos - num_columns; i >= (signed)pageStart; i -= num_columns) {
        if (i >= 0 && IsItemValidAndEnabled(i))
          return true;
      }
    } else { // DOWN
      for (unsigned i = focusPos + num_columns; i < pageEnd && i < items.size(); i += num_columns) {
        if (IsItemValidAndEnabled(i))
          return true;
      }
    }
  }

  return false;
}

signed
GridView::FindNextInRow(signed currIndex, Direction direction, bool cycle) const noexcept
{
  PageBounds bounds = GetPageBoundsForIndex(currIndex);
  PagePosition pos = GetPagePosition(currIndex);
  unsigned pageStart = bounds.pageStart;
  unsigned pageEnd = bounds.pageEnd;
  unsigned rowNum = pos.rowNum;
  unsigned rowStart = pageStart + rowNum * num_columns;
  unsigned rowEnd = std::min(rowStart + num_columns, pageEnd);

  if (direction == Direction::LEFT) {
    for (signed i = currIndex - 1; i >= (signed)rowStart; i--) {
      if (i >= 0 && IsItemValidAndEnabled(i))
        return i;
    }
    if (cycle) {
      for (signed i = rowEnd - 1; i >= (signed)rowStart; i--) {
        if (i >= 0 && i < (signed)pageEnd && IsItemValidAndEnabled(i))
          return i;
      }
    }
  } else {
    for (unsigned i = currIndex + 1; i < rowEnd && i < items.size(); i++) {
      if (IsItemValidAndEnabled(i))
        return i;
    }
    if (cycle) {
      for (unsigned i = rowStart; i < rowEnd && i < items.size(); i++) {
        if (IsItemValidAndEnabled(i))
          return i;
      }
    }
  }
  return -1;
}

signed
GridView::FindNextInColumn(signed currIndex, Direction direction, bool cycle) const noexcept
{
  PageBounds bounds = GetPageBoundsForIndex(currIndex);
  PagePosition pos = GetPagePosition(currIndex);
  unsigned pageStart = bounds.pageStart;
  unsigned pageEnd = bounds.pageEnd;
  unsigned colNum = pos.colNum;

  if (direction == Direction::UP) {
    for (signed i = currIndex - num_columns; i >= (signed)pageStart; i -= num_columns) {
      if (i >= 0 && IsItemValid(i) && IsItemInColumn(i, colNum) && items[i]->IsEnabled())
        return i;
    }
    if (cycle) {
      // Find the actual last item in this column by searching backwards from pageEnd
      for (signed i = (signed)pageEnd - 1; i >= (signed)pageStart; i--) {
        if (i >= 0 && IsItemValid(i) && IsItemInColumn(i, colNum) && items[i]->IsEnabled())
          return i;
      }
    }
  } else {
    for (unsigned i = currIndex + num_columns; i < pageEnd && i < items.size(); i += num_columns) {
      if (IsItemValid(i) && IsItemInColumn(i, colNum) && items[i]->IsEnabled())
        return i;
    }
    if (cycle) {
      for (unsigned i = pageStart + colNum; i < pageEnd && i < items.size(); i += num_columns) {
        if (IsItemValid(i) && IsItemInColumn(i, colNum) && items[i]->IsEnabled())
          return i;
      }
    }
  }
  return -1;
}

GridView::PageBounds
GridView::GetPageBounds(unsigned page) const noexcept
{
  unsigned pageSize = GetPageSize();
  unsigned lastPage = items.size() / pageSize;
  unsigned currentPageSize = page == lastPage
    ? items.size() % pageSize
    : pageSize;
  unsigned pageStart = page * pageSize;
  unsigned pageEnd = pageStart + currentPageSize;
  return {pageStart, pageEnd, currentPageSize, lastPage};
}

GridView::PageBounds
GridView::GetPageBoundsForIndex(signed index) const noexcept
{
  if (index < 0)
    return GetPageBounds(0);
  unsigned pageSize = GetPageSize();
  unsigned page = index / pageSize;
  return GetPageBounds(page);
}

GridView::PagePosition
GridView::GetPagePosition(signed index) const noexcept
{
  if (index < 0)
    return {0, 0, 0};
  unsigned pageSize = GetPageSize();
  unsigned pagePos = index % pageSize;
  return {pagePos, pagePos / num_columns, pagePos % num_columns};
}

signed
GridView::FindFirstEnabledInRange(unsigned start, unsigned end) const noexcept
{
  for (unsigned i = start; i < end && i < items.size(); ++i) {
    if (IsItemValidAndEnabled(i))
      return i;
  }
  return -1;
}

void
GridView::SaveRowPosition(unsigned page) noexcept
{
  signed focusPos = GetIndexOfItemInFocus();
  if (focusPos == -1)
    return;

  PagePosition pos = GetPagePosition(focusPos);
  unsigned rowNum = pos.rowNum;

  while (saved_row_per_page.size() <= page)
    saved_row_per_page.append(0);
  saved_row_per_page[page] = rowNum;
}

unsigned
GridView::GetSavedRowPosition(unsigned page) const noexcept
{
  if (page < saved_row_per_page.size())
    return saved_row_per_page[page];

  signed focusPos = GetIndexOfItemInFocus();
  if (focusPos != -1) {
    PagePosition pos = GetPagePosition(focusPos);
    return pos.rowNum;
  }

  return 0;
}

signed
GridView::CalculateTargetPosition(unsigned pageStart, unsigned pageEnd,
                                   unsigned preferredRow,
                                   Direction fromDirection) const noexcept
{
  unsigned currentPageSize = pageEnd - pageStart;
  unsigned maxRowsOnPage = DivideRoundUp(currentPageSize, num_columns);
  
  if (preferredRow >= maxRowsOnPage && maxRowsOnPage > 0) {
    preferredRow = maxRowsOnPage - 1;
  }

  signed targetPos = -1;
  if (fromDirection == Direction::LEFT) {
    targetPos = pageStart + (preferredRow + 1) * num_columns - 1;
    if (targetPos >= (signed)pageEnd)
      targetPos = pageEnd - 1;
  } else {
    targetPos = pageStart + preferredRow * num_columns;
    if (targetPos >= (signed)items.size()) {
      targetPos = pageStart + currentPageSize
        - (currentPageSize - 1) % num_columns - 1;
    }
  }

  if (targetPos >= 0 && targetPos >= (signed)pageStart && targetPos < (signed)pageEnd &&
      IsItemValidAndEnabled(targetPos)) {
    return targetPos;
  }

  signed rowPos = FindEnabledInRow(pageStart, preferredRow, pageEnd);
  if (rowPos != -1)
    return rowPos;

  if (maxRowsOnPage > 0) {
    unsigned lastRow = maxRowsOnPage - 1;
    rowPos = FindEnabledInRow(pageStart, lastRow, pageEnd);
    if (rowPos != -1)
      return rowPos;
  }

  return -1;
}

signed
GridView::FocusPositionOnPage(unsigned preferredRow, Direction fromDirection) noexcept
{
  PageBounds bounds = GetPageBounds(current_page);

  return CalculateTargetPosition(bounds.pageStart, bounds.pageEnd, preferredRow,
                                  fromDirection);
}

void
GridView::ShowNextPage(Direction direction)
{
  unsigned pageSize = GetPageSize();
  if (pageSize == 0)
    return;

  signed focusPos = GetIndexOfItemInFocus();
  unsigned savedRow = 0;
  unsigned currentRow = 0;
  
  if (focusPos != -1) {
    PagePosition pos = GetPagePosition(focusPos);
    currentRow = pos.rowNum;
    savedRow = currentRow;
  }

  unsigned oldPage = current_page;
  bool jumpedToLastRow = false;
  bool jumpingBackFromLastRow = false;

  if (!IsHorizontal(direction))
    return;

  // Calculate target page and check for row restoration
  PageBounds oldBounds = GetPageBounds(oldPage);
  unsigned oldMaxRows = DivideRoundUp(oldBounds.currentPageSize, num_columns);
  unsigned targetPage;
  
  if (direction == Direction::LEFT) {
    targetPage = (oldPage == 0) ? oldBounds.lastPage : (oldPage - 1);
  } else { // RIGHT
    targetPage = (oldPage >= oldBounds.lastPage) ? 0 : (oldPage + 1);
  }
  
  // Check if we should restore the saved row from previous page
  if (oldMaxRows > 0 && currentRow == oldMaxRows - 1 && 
      targetPage == previous_page && saved_row_from_previous_page > 0) {
    jumpingBackFromLastRow = true;
    savedRow = saved_row_from_previous_page;
  }
  
  current_page = targetPage;

  PageBounds bounds = GetPageBounds(current_page);
  unsigned maxRowsOnPage = DivideRoundUp(bounds.currentPageSize, num_columns);
  unsigned preferredRow = savedRow;
  
  if (preferredRow >= maxRowsOnPage && maxRowsOnPage > 0) {
    preferredRow = maxRowsOnPage - 1;
    jumpedToLastRow = true;
  }

  // Update previous page tracking
  previous_page = oldPage;
  if (jumpedToLastRow && !jumpingBackFromLastRow) {
    saved_row_from_previous_page = savedRow;
  } else if (!jumpingBackFromLastRow) {
    saved_row_from_previous_page = 0;
  }

  signed newPos = FocusPositionOnPage(preferredRow, direction);

  // Fallback: search current page, then all pages if needed
  if (newPos == -1) {
    newPos = FindFirstEnabledInRange(bounds.pageStart, bounds.pageEnd);

    if (newPos == -1) {
      // Search all pages for any enabled item
      for (unsigned page = 0; page <= bounds.lastPage; ++page) {
        PageBounds pageBounds = GetPageBounds(page);
        newPos = FindFirstEnabledInRange(pageBounds.pageStart, pageBounds.pageEnd);
        if (newPos != -1) {
          current_page = page;
          break;
        }
      }
    }
  }

  if (newPos != -1 && IsItemValid(newPos)) {
    items[newPos]->SetFocus();
    if (focusPos != -1 && IsItemValid(focusPos) && items[newPos]->HasFocus()) {
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
  PageBounds bounds = GetPageBoundsForIndex(currIndex);
  PagePosition pos = GetPagePosition(currIndex);
  unsigned pageSize = GetPageSize();
  unsigned colNum = pos.colNum;
  unsigned rowNum = pos.rowNum;
  unsigned pageStart = bounds.pageStart;
  unsigned pageEnd = bounds.pageEnd;
  bool singlePage = (items.size() <= pageSize);
  unsigned rowStart = pageStart + rowNum * num_columns;
  unsigned rowEnd = std::min(rowStart + num_columns, pageEnd);

  if (IsHorizontal(direction)) {
    // LEFT or RIGHT: iterate through row
    if (direction == Direction::LEFT) {
      for (signed i = currIndex - 1; i >= (signed)rowStart; i--) {
        if (i >= 0 && IsItemValidAndEnabled(i) && IsItemInRow(i, rowNum))
          return i;
      }
      if (singlePage) {
        for (signed i = rowEnd - 1; i >= (signed)rowStart; i--) {
          if (i >= 0 && i < (signed)pageEnd && IsItemValidAndEnabled(i) && IsItemInRow(i, rowNum))
            return i;
        }
      }
    } else { // RIGHT
      for (unsigned i = currIndex + 1; i < rowEnd && i < items.size(); i++) {
        if (IsItemValidAndEnabled(i) && IsItemInRow(i, rowNum))
          return i;
      }
      if (singlePage) {
        for (unsigned i = rowStart; i < rowEnd && i < items.size(); i++) {
          if (IsItemValidAndEnabled(i) && IsItemInRow(i, rowNum))
            return i;
        }
      }
    }
  } else {
    // UP or DOWN: iterate through column
    if (direction == Direction::UP) {
      for (signed i = currIndex - num_columns; i >= (signed)pageStart; i -= num_columns) {
        if (i >= 0 && IsItemValidAndEnabled(i) && IsItemInColumn(i, colNum))
          return i;
      }
      
      unsigned maxRowsOnPage = DivideRoundUp(bounds.currentPageSize, num_columns);
      if (singlePage && maxRowsOnPage > 0) {
        unsigned lastRow = maxRowsOnPage - 1;
        for (signed i = pageStart + lastRow * num_columns + colNum; 
             i >= (signed)(pageStart + lastRow * num_columns); i--) {
          if (i >= (signed)pageStart && i < (signed)pageEnd && IsItemValidAndEnabled(i) && IsItemInColumn(i, colNum))
            return i;
        }
      }
    } else { // DOWN
      for (unsigned i = currIndex + num_columns; i < pageEnd && i < items.size(); i += num_columns) {
        if (IsItemValidAndEnabled(i) && IsItemInColumn(i, colNum))
          return i;
      }
      
      if (singlePage) {
        for (unsigned i = pageStart + colNum; i < pageStart + num_columns && i < pageEnd; i++) {
          if (IsItemValidAndEnabled(i) && IsItemInColumn(i, colNum))
            return i;
        }
      }
    }
  }

  return -1;
}

signed
GridView::GetNextItemIndex(unsigned currIndex, Direction direction) const
{
  PageBounds bounds = GetPageBoundsForIndex(currIndex);
  PagePosition pos = GetPagePosition(currIndex);
  unsigned pageSize = GetPageSize();
  unsigned colNum = pos.colNum;
  unsigned rowNum = pos.rowNum;
  signed nextPos = -1;

  unsigned pageStart = bounds.pageStart;
  unsigned pageEnd = bounds.pageEnd;
  bool singlePage = (items.size() <= pageSize);
  unsigned rowStart = pageStart + rowNum * num_columns;
  unsigned rowEnd = std::min(rowStart + num_columns, pageEnd);

  if (IsHorizontal(direction)) {
    // LEFT or RIGHT: row-based navigation
    if (direction == Direction::LEFT) {
      if (colNum > 0) {
        nextPos = currIndex - 1;
      } else if (singlePage) {
        nextPos = rowEnd - 1;
        if (nextPos >= (signed)pageEnd)
          nextPos = pageEnd - 1;
      }
    } else { // RIGHT
      if (colNum + 1 < num_columns && currIndex + 1 < items.size()) {
        if (currIndex + 1 < rowEnd)
          nextPos = currIndex + 1;
      } else if (singlePage) {
        nextPos = rowStart;
      }
    }
  } else {
    // UP or DOWN: column-based navigation
    unsigned maxRowsOnPage = DivideRoundUp(bounds.currentPageSize, num_columns);
    
    if (direction == Direction::UP) {
      if (rowNum > 0) {
        nextPos = currIndex - num_columns;
      } else if (maxRowsOnPage > 0 && singlePage) {
        unsigned lastRow = maxRowsOnPage - 1;
        unsigned lastRowStart = pageStart + lastRow * num_columns;
        unsigned lastRowEnd = std::min(lastRowStart + num_columns, pageEnd);
        nextPos = lastRowStart + colNum;
        if (nextPos >= (signed)lastRowEnd)
          nextPos = lastRowEnd - 1;
      }
    } else { // DOWN
      if (rowNum + 1 < maxRowsOnPage) {
        signed nextRowPos = currIndex + num_columns;
        if (nextRowPos < (signed)pageEnd)
          nextPos = nextRowPos;
      } else if (maxRowsOnPage > 0 && singlePage) {
        nextPos = pageStart + colNum;
        if ((unsigned)nextPos >= pageEnd)
          nextPos = pageStart;
      }
    }
  }

  return nextPos;
}

void
GridView::MoveFocus(Direction direction)
{
  signed focusPos = GetIndexOfItemInFocus();

  if (focusPos == -1) {
    ShowNextPage(direction);
    return;
  }

  unsigned pageSize = GetPageSize();
  bool singlePage = (items.size() <= pageSize);

  signed newFocusPos = -1;

  if (direction == Direction::LEFT || direction == Direction::RIGHT) {
    newFocusPos = FindNextInRow(focusPos, direction, singlePage);
    
    if (newFocusPos == -1) {
      if (singlePage) {
        return;
      }
      ShowNextPage(direction);
      return;
    }
  } else {
    newFocusPos = FindNextInColumn(focusPos, direction, true);
    
    if (newFocusPos == -1) {
      return;
    }
  }

  if (newFocusPos == focusPos) {
    return;
  }

  if (newFocusPos != -1 && newFocusPos >= 0 && IsItemValid(newFocusPos)) {
    items[newFocusPos]->SetFocus();
    RefreshLayout();
  }
}

void
GridView::OnResize(PixelSize new_size) noexcept
{
  PanelControl::OnResize(new_size);
  RefreshLayout();
}
