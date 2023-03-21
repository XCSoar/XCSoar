// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
