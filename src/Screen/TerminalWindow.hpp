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

#ifndef XCSOAR_SCREEN_TERMINAL_WINDOW_HPP
#define XCSOAR_SCREEN_TERMINAL_WINDOW_HPP

#include "Screen/PaintWindow.hpp"
#include "Screen/Point.hpp"
#include "Util/AllocatedGrid.hxx"

#include <tchar.h>

struct TerminalLook;

class TerminalWindow : public PaintWindow {
  const TerminalLook &look;

  unsigned cursor_x, cursor_y;
  PixelSize cell_size;

  AllocatedGrid<TCHAR> data;

public:
  TerminalWindow(const TerminalLook &_look):look(_look) {}

  void Write(const char *p, size_t length);
  void Clear();

private:
  void Scroll();
  void NewLine();
  void Advance();

protected:
  virtual void OnCreate() override;
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnPaint(Canvas &canvas) override;
  virtual void OnPaint(Canvas &canvas, const PixelRect &dirty) override;
};

#endif
