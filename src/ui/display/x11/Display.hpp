/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#pragma once

struct _XDisplay;
struct __GLXFBConfigRec;
struct __GLXcontextRec;
struct PixelSize;

namespace X11 {

class Display {
  _XDisplay *const display;

#ifdef USE_GLX
  __GLXFBConfigRec **fb_cfg;

  __GLXcontextRec *glx_context;
#endif

public:
  /**
   * Throws on error.
   */
  Display();

  ~Display() noexcept;

  auto GetXDisplay() const noexcept {
    return display;
  }

  [[gnu::pure]]
  PixelSize GetSize() const noexcept;

  /**
   * Returns the display size in mm.
   */
  [[gnu::pure]]
  PixelSize GetSizeMM() const noexcept;

#ifdef USE_GLX
  auto *GetFBConfig() const noexcept {
    return *fb_cfg;
  }

  auto *GetGLXContext() const noexcept {
    return glx_context;
  }
#endif
};

} // namespace X11
