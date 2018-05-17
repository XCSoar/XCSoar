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

#ifndef XCSOAR_FORM_GRIDVIEW_HPP
#define XCSOAR_FORM_GRIDVIEW_HPP

#include "Panel.hpp"
#include "Util/StaticArray.hxx"

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

  gcc_pure
  signed GetIndexOfItemInFocus() const;

  void MoveFocus(Direction direction);
  void ShowNextPage(Direction direction = Direction::RIGHT);
  void RefreshLayout();

private:
  gcc_pure
  signed GetNextItemIndex(unsigned currIndex, Direction direction) const;

  gcc_pure
  signed GetNextEnabledItemIndex(signed currIndex, Direction direction) const;

  /* virtual methods from class Window */
  void OnResize(PixelSize new_size) override;
};

#endif
