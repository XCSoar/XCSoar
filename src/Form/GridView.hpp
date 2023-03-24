// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Panel.hpp"
#include "util/StaticArray.hxx"

class GridView : public PanelControl {
public:
  static constexpr unsigned MAX_ITEMS = 32;

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

  void MoveFocus(Direction direction);
  void ShowNextPage(Direction direction = Direction::RIGHT);
  void RefreshLayout();

private:
  [[gnu::pure]]
  signed GetNextItemIndex(unsigned currIndex, Direction direction) const;

  [[gnu::pure]]
  signed GetNextEnabledItemIndex(signed currIndex, Direction direction) const;

  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) noexcept override;
};
