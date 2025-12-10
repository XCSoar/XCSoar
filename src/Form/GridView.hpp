// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Panel.hpp"
#include "util/StaticArray.hxx"

class GridView : public PanelControl {
public:
  static constexpr unsigned MAX_ITEMS = 64;

  enum class Direction
  {
    LEFT,
    RIGHT,
    UP,
    DOWN
  };

private:
  StaticArray<Window *, MAX_ITEMS> items;

  unsigned column_width;
  unsigned row_height;

  unsigned num_columns;
  unsigned num_rows;
  unsigned current_page;
  unsigned previous_page;
  unsigned saved_row_from_previous_page;

  StaticArray<unsigned, 16> saved_row_per_page;

public:
  void Create(ContainerWindow &parent, const DialogLook &look,
              const PixelRect &rc, const WindowStyle style,
              unsigned column_width, unsigned row_height);

  void AddItem(Window &w) {
    items.push_back(&w);
  }

  unsigned GetColumnWidth() const {
    return column_width;
  }

  unsigned GetRowHeight() const {
    return row_height;
  }

  void SetColumnWidth(unsigned _column_width) noexcept {
    column_width = _column_width;
  }

  unsigned GetCurrentPage() const {
    return current_page;
  }

  unsigned GetNumColumns() const {
    return num_columns;
  }

  unsigned GetNumRows() const {
    return num_rows;
  }

  [[gnu::pure]]
  signed GetIndexOfItemInFocus() const;

  [[gnu::pure]]
  unsigned GetPageSize() const noexcept {
    return num_columns * num_rows;
  }

  void MoveFocus(Direction direction);
  void ShowNextPage(Direction direction = Direction::RIGHT);
  void RefreshLayout();

private:
  [[gnu::pure]]
  signed GetNextItemIndex(unsigned currIndex, Direction direction) const;

  [[gnu::pure]]
  signed GetNextEnabledItemIndex(signed currIndex, Direction direction) const;

  signed FindEnabledInRow(unsigned pageStart, unsigned rowNum,
                          unsigned pageEnd) const noexcept;
  signed FindEnabledInColumn(unsigned pageStart, unsigned colNum,
                             unsigned pageEnd) const noexcept;
  bool HasEnabledInRow(unsigned pageStart, unsigned rowNum,
                       unsigned pageEnd) const noexcept;
  bool HasEnabledInDirection(unsigned focusPos, Direction direction) const noexcept;
  
  signed FindNextInRow(signed currIndex, Direction direction, bool cycle) const noexcept;
  signed FindNextInColumn(signed currIndex, Direction direction, bool cycle) const noexcept;
  
  [[gnu::const]]
  static bool IsHorizontal(Direction direction) noexcept {
    return direction == Direction::LEFT || direction == Direction::RIGHT;
  }
  
  [[gnu::const]]
  static bool IsVertical(Direction direction) noexcept {
    return direction == Direction::UP || direction == Direction::DOWN;
  }
  
  struct PageBounds {
    unsigned pageStart;
    unsigned pageEnd;
    unsigned currentPageSize;
    unsigned lastPage;
  };
  PageBounds GetPageBounds(unsigned page) const noexcept;
  PageBounds GetPageBoundsForIndex(signed index) const noexcept;
  
  struct PagePosition {
    unsigned pagePos;
    unsigned rowNum;
    unsigned colNum;
  };
  PagePosition GetPagePosition(signed index) const noexcept;
  
  signed FindFirstEnabledInRange(unsigned start, unsigned end) const noexcept;
  
  [[gnu::pure]]
  bool IsItemValid(unsigned i) const noexcept {
    return i < items.size() && items[i] != nullptr;
  }
  
  [[gnu::pure]]
  bool IsItemValidAndEnabled(unsigned i) const noexcept {
    return IsItemValid(i) && items[i]->IsEnabled();
  }
  
  [[gnu::pure]]
  bool IsItemValidAndHasFocus(unsigned i) const noexcept {
    return IsItemValid(i) && items[i]->HasFocus();
  }
  
  [[gnu::pure]]
  bool IsItemInColumn(unsigned i, unsigned colNum) const noexcept {
    unsigned pageSize = GetPageSize();
    unsigned iPagePos = i % pageSize;
    return iPagePos % num_columns == colNum;
  }
  
  [[gnu::pure]]
  bool IsItemInRow(unsigned i, unsigned rowNum) const noexcept {
    unsigned pageSize = GetPageSize();
    unsigned iPagePos = i % pageSize;
    return iPagePos / num_columns == rowNum;
  }
  
  void SaveRowPosition(unsigned page) noexcept;
  unsigned GetSavedRowPosition(unsigned page) const noexcept;
  signed CalculateTargetPosition(unsigned pageStart, unsigned pageEnd,
                                 unsigned preferredRow,
                                 Direction fromDirection) const noexcept;
  signed FocusPositionOnPage(unsigned preferredRow, Direction fromDirection) noexcept;

  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;
};
