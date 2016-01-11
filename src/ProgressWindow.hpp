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

#ifndef XCSOAR_SCREEN_PROGRESS_WINDOW_HXX
#define XCSOAR_SCREEN_PROGRESS_WINDOW_HXX

#include "Screen/ContainerWindow.hpp"
#include "Screen/ProgressBar.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Color.hpp"
#include "Gauge/LogoView.hpp"
#include "Util/StaticString.hxx"

/**
 * The XCSoar splash screen with a progress bar.
 */
class ProgressWindow : public ContainerWindow {
  Color background_color;

  Bitmap bitmap_progress_border;

#ifndef USE_WINUSER
  Font font;
#endif

  LogoView logo;

  StaticString<128> message;

  ProgressBar progress_bar;

  unsigned text_height;

  PixelRect logo_position, message_position;
  PixelRect bottom_position, progress_bar_position;

public:
  explicit ProgressWindow(ContainerWindow &parent);

  void SetMessage(const TCHAR *text);

  void SetRange(unsigned min_value, unsigned max_value);
  void SetStep(unsigned size);
  void SetValue(unsigned value);
  void Step();

private:
  void UpdateLayout(PixelRect rc);

protected:
  virtual void OnResize(PixelSize new_size) override;
  virtual void OnPaint(Canvas &canvas) override;
};

#endif
