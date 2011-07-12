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

#ifndef XCSOAR_SCREEN_PROGRESS_WINDOW_HXX
#define XCSOAR_SCREEN_PROGRESS_WINDOW_HXX

#include "ContainerWindow.hpp"
#include "TextWindow.hpp"
#include "ProgressBar.hpp"
#include "Bitmap.hpp"
#include "Gauge/LogoView.hpp"

/**
 * The XCSoar splash screen with a progress bar.
 */
class ProgressWindow : public ContainerWindow {
  Color background_color;
  Brush background_brush;

  Bitmap bitmap_progress_border;

#ifndef USE_GDI
  Font font;
#endif

  LogoView logo;

  TextWindow message;

  ProgressBar progress_bar;
  unsigned position;

  unsigned text_height;
  unsigned progress_border_height;
public:
  explicit ProgressWindow(ContainerWindow &parent);

  void set_message(const TCHAR *text);

  void set_range(unsigned min_value, unsigned max_value);
  void set_step(unsigned size);
  void set_pos(unsigned value);
  void step();

protected:
  virtual void on_paint(Canvas &canvas);
  virtual const Brush *on_color(Window &window, Canvas &canvas);
};

#endif
