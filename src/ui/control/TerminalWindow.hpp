// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "ui/dim/Size.hpp"
#include "util/AllocatedGrid.hxx"

#include <tchar.h>

struct TerminalLook;

class TerminalWindow : public PaintWindow {
  const TerminalLook &look;

  unsigned cursor_x, cursor_y;
  PixelSize cell_size;

  AllocatedGrid<char> data;

public:
  TerminalWindow(const TerminalLook &_look):look(_look) {}

  void Write(const char *p, size_t length);
  void Clear();

private:
  void Scroll();
  void NewLine();
  void Advance();

protected:
  void OnCreate() override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
  void OnPaint(Canvas &canvas, const PixelRect &dirty) noexcept override;
};
