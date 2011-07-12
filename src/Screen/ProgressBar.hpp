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

#ifndef XCSOAR_SCREEN_PROGRESS_BAR_HXX
#define XCSOAR_SCREEN_PROGRESS_BAR_HXX

#include "Window.hpp"

class ProgressBarStyle : public WindowStyle {
public:
  ProgressBarStyle() {}
  ProgressBarStyle(const WindowStyle &_style):WindowStyle(_style) {}

  void vertical();
  void smooth();
};

class ProgressBar : public Window {
#ifndef USE_GDI
  unsigned min_value, max_value, value, step_size;

public:
  ProgressBar():min_value(0), max_value(0), value(0), step_size(1) {}
#endif

public:
  void set(ContainerWindow &parent,
           int left, int top, unsigned width, unsigned height,
           const ProgressBarStyle style=ProgressBarStyle());

  void set_range(unsigned min_value, unsigned max_value);
  void set_position(unsigned value);
  void set_step(unsigned size);
  void step();

#ifndef USE_GDI
protected:
  virtual void on_paint(Canvas &canvas);
#endif
};

#endif
