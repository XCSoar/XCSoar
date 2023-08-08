// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "Hardware/CPU.hpp"

#ifdef ANDROID
#include "Android/Main.hpp"
#include "Android/NativeView.hpp"
#include "ui/event/android/Loop.hpp"
#include "util/ScopeExit.hxx"
#elif defined(ENABLE_SDL)
#include "ui/event/sdl/Event.hpp"
#include "ui/event/sdl/Loop.hpp"
#else
#include "ui/event/poll/Loop.hpp"
#include "ui/event/shared/Event.hpp"
#endif

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Dynamic.hpp" // for GLExt::discard_framebuffer
#endif

#ifdef DRAW_MOUSE_CURSOR
#include "Screen/Layout.hpp"
#endif

namespace UI {

TopWindow::~TopWindow() noexcept
{
#ifdef ANDROID
  native_view->SetPointer(Java::GetEnv(), nullptr);
#endif

  delete screen;
}

void
TopWindow::Create([[maybe_unused]] const TCHAR *text, PixelSize size,
                  TopWindowStyle style)
{
  invalidated = true;

#if defined(USE_X11) || defined(USE_WAYLAND) || defined(ENABLE_SDL)
  CreateNative(text, size, style);
#endif

  delete screen;
  screen = nullptr;

#ifdef ENABLE_SDL
  screen = new TopCanvas(display, window);
#elif defined(USE_GLX)
  screen = new TopCanvas(display, x_window);
#elif defined(USE_X11)
  screen = new TopCanvas(display, x_window);
#elif defined(USE_WAYLAND)
  screen = new TopCanvas(display, native_window);
#elif defined(USE_VFB)
  screen = new TopCanvas(display, size);
#else
  screen = new TopCanvas(display);
#endif

#ifdef SOFTWARE_ROTATE_DISPLAY
  size = screen->SetDisplayOrientation(style.GetInitialOrientation());
#elif defined(USE_MEMORY_CANVAS)
  size = screen->GetSize();
#endif
  ContainerWindow::Create(nullptr, PixelRect{size}, style);
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopWindow::SetDisplayOrientation(DisplayOrientation orientation) noexcept
{
  assert(screen != nullptr);

  Resize(screen->SetDisplayOrientation(orientation));
}

#endif

void
TopWindow::CancelMode() noexcept
{
  OnCancelMode();
}

void
TopWindow::Invalidate() noexcept
{
  invalidated = true;
}

#ifdef DRAW_MOUSE_CURSOR

inline void
TopWindow::DrawMouseCursor(Canvas &canvas) noexcept
{
  const auto m = event_queue->GetMousePosition();
  const int shortDistance = Layout::Scale(cursor_size * 4);
  const int longDistance = Layout::Scale(cursor_size * 6);

  const BulkPixelPoint p[] = {
    { m.x, m.y },
    { m.x + shortDistance, m.y + shortDistance },
    { m.x, m.y + longDistance },
  };

  if (invert_cursor_colors) {
    canvas.SelectWhitePen(cursor_size);
    canvas.SelectBlackBrush();
  } else {
    canvas.SelectBlackPen(cursor_size);
    canvas.SelectWhiteBrush();
  }
  canvas.DrawTriangleFan(p, std::size(p));
}

#endif

void
TopWindow::Expose() noexcept
{
#ifdef HAVE_CPU_FREQUENCY
  const ScopeLockCPU cpu;
#endif

  if (auto canvas = screen->Lock(); canvas.IsDefined()) {
    OnPaint(canvas);

#ifdef DRAW_MOUSE_CURSOR
    DrawMouseCursor(canvas);
#endif

    screen->Unlock();
  }

  screen->Flip();

#if defined(ENABLE_OPENGL) && defined(GL_EXT_discard_framebuffer)
  /* tell the GPU that we won't be needing the frame buffer contents
     again which can increase rendering performance; see
     https://registry.khronos.org/OpenGL/extensions/EXT/EXT_discard_framebuffer.txt */
  if (GLExt::discard_framebuffer != nullptr) {
    static constexpr GLenum attachments[3] = {
      GL_COLOR_EXT,
      GL_DEPTH_EXT,
      GL_STENCIL_EXT
    };

    GLExt::discard_framebuffer(GL_FRAMEBUFFER, std::size(attachments),
                               attachments);
  }
#endif
}

void
TopWindow::Refresh() noexcept
{
  if (!screen->IsReady())
    /* the application is paused/suspended, and we don't have an
       OpenGL surface - ignore all drawing requests */
    return;

#ifdef USE_X11
  if (!IsVisible())
    /* don't bother to invoke the renderer if we're not visible on the
       X11 display */
    return;
#endif

  if (!invalidated)
    return;

  invalidated = false;

  Expose();
}

bool
TopWindow::OnActivate() noexcept
{
  return false;
}

bool
TopWindow::OnDeactivate() noexcept
{
  return false;
}

bool
TopWindow::OnClose() noexcept
{
  Destroy();
  return true;
}

int
TopWindow::RunEventLoop() noexcept
{
#ifdef ANDROID
  BeginRunning();
  AtScopeExit(this) { EndRunning(); };
#endif

  Refresh();

  EventLoop loop(*event_queue, *this);
  Event event;
  while (IsDefined() && loop.Get(event))
    loop.Dispatch(event);

  return 0;
}

void
TopWindow::PostQuit() noexcept
{
  event_queue->Quit();
}

} // namespace UI
