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

#ifndef XCSOAR_SCREEN_PROGRESS_BAR_HXX
#define XCSOAR_SCREEN_PROGRESS_BAR_HXX

#include "PaintWindow.hpp"

class ProgressBar : public PaintWindow {
  unsigned min_value = 0, max_value = 0, value = 0, step_size = 1;

public:
  void SetRange(unsigned min_value, unsigned max_value);

  unsigned GetValue() const {
    return value;
  }

  void SetValue(unsigned value);
  void SetStep(unsigned size);
  void Step();

protected:
  void OnPaint(Canvas &canvas) override;
};

#endif
