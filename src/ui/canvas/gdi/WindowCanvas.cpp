// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ui/canvas/WindowCanvas.hpp"
#include "ui/window/PaintWindow.hpp"

WindowCanvas::WindowCanvas(PaintWindow &window)
  :Canvas(::GetDC(window), window.GetSize()),
   wnd(window) {}
