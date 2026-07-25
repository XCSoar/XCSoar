// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "../TopWindow.hpp"
#include "ui/canvas/Features.hpp" // for DRAW_MOUSE_CURSOR
#include "ui/canvas/custom/TopCanvas.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/Queue.hpp"
#include "ui/event/Globals.hpp"
#include "Hardware/CPU.hpp"

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

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

#if defined(DRAW_MOUSE_CURSOR) || defined(DRAW_REDRAW_COUNTER)
#include "Screen/Layout.hpp"
#endif

#ifdef DRAW_REDRAW_COUNTER
#include "Look/FontDescription.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/canvas/Font.hpp"
#include <fmt/format.h>
#include <chrono>
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
TopWindow::Create([[maybe_unused]] const char *text, PixelSize size,
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
#ifdef USE_POLL_EVENT
  if (event_queue != nullptr) {
    event_queue->SetDisplayOrientation(style.GetInitialOrientation());
    PixelSize native_size = size;
#if defined(ENABLE_OPENGL) && defined(USE_LIBINPUT)
    if (!event_queue->UsesSystemRotatedInput() &&
        AreAxesSwapped(style.GetInitialOrientation()))
      native_size = {size.height, size.width};
#endif
    event_queue->SetScreenSize(native_size);
  }
#endif
#elif defined(USE_MEMORY_CANVAS)
  size = screen->GetSize();
#elif defined(ENABLE_OPENGL)
  // On HiDPI displays, the drawable size may differ from window size
  PixelSize native_size = screen->GetNativeSize();
  // On Android, surface might not be ready yet, so GetNativeSize() may return 0x0
  // In that case, use the size passed to Create() (which should have a fallback)
  if (native_size.width > 0 && native_size.height > 0)
    size = native_size;
  // else keep the original size (which should be from SystemWindowSize() with fallback)
#endif
  ContainerWindow::Create(nullptr, PixelRect{size}, style);
}

#ifdef SOFTWARE_ROTATE_DISPLAY

void
TopWindow::SetDisplayOrientation(DisplayOrientation orientation) noexcept
{
  assert(screen != nullptr);

  const PixelSize new_size = screen->SetDisplayOrientation(orientation);
  const bool resize_needed = new_size != GetSize();

#ifdef ENABLE_OPENGL
  /* Re-read the current drawable size after orientation changes.
     On some UNIX backends, output/orientation changes don't always
     deliver a fresh configure event immediately. */
  const PixelSize native_size = screen->GetNativeSize();
  if (native_size.width > 0 && native_size.height > 0 &&
      screen->CheckResize(native_size)) {
    Resize(screen->GetSize());
    return;
  }
#endif

  if (!resize_needed)
    BumpRenderStateToken();

  Resize(new_size);
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

#ifdef DRAW_REDRAW_COUNTER

inline void
TopWindow::DrawRedrawCounter(Canvas &canvas) noexcept
{
  using namespace std::chrono;

  ++redraw_count;
  ++hz_window_frames;

  const auto now = steady_clock::now();
  if (hz_window_start.time_since_epoch().count() == 0)
    hz_window_start = now;

  const auto elapsed = now - hz_window_start;
  if (elapsed >= seconds{1}) {
    const double seconds_elapsed =
      duration<double>(elapsed).count();
    if (seconds_elapsed > 0)
      redraw_hz = hz_window_frames / seconds_elapsed;
    hz_window_start = now;
    hz_window_frames = 0;
  }

  static Font font;
  if (!font.IsDefined()) {
    try {
      font.Load(FontDescription(Layout::FontScale(12)));
    } catch (...) {
      return;
    }
  }

  const auto text = fmt::format("R:{}  {:.1f}/s",
                                redraw_count, redraw_hz);
  const PixelSize text_size = font.TextSize(text);
  const int pad = Layout::Scale(2);
  const PixelRect box{
    pad,
    pad,
    pad + int(text_size.width) + pad * 2,
    pad + int(text_size.height) + pad * 2,
  };

  canvas.DrawFilledRectangle(box, COLOR_WHITE);
  canvas.Select(font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundColor(COLOR_WHITE);
  canvas.DrawText({box.left + pad, box.top + pad}, text);
}

#endif

void
TopWindow::Expose() noexcept
{
#ifdef HAVE_CPU_FREQUENCY
  const ScopeLockCPU cpu;
#endif

#if defined(ENABLE_SDL) && defined(USE_MEMORY_CANVAS)
  // Process any pending resize BEFORE locking the canvas
  screen->ProcessPendingResize();
#endif

  if (auto canvas = screen->Lock(); canvas.IsDefined()) {
    OnPaint(canvas);

#ifdef DRAW_MOUSE_CURSOR
    if (std::chrono::steady_clock::now() < cursor_visible_until)
      DrawMouseCursor(canvas);
#endif

#ifdef DRAW_REDRAW_COUNTER
    DrawRedrawCounter(canvas);
#endif

    screen->Unlock();
  }

  screen->Flip();

#if defined(ENABLE_OPENGL) && defined(GL_EXT_discard_framebuffer) && \
  (defined(ANDROID) || defined(MESA_KMS))
  /* On mobile/KMS style backends, discarding the previous window
     contents can save bandwidth.  Desktop EGL/GLX compositors may
     still read from the just-swapped window surface, so avoid this
     optimisation there. */
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

#if defined(USE_X11) || defined(USE_WAYLAND)
  if (!IsVisible())
    /* don't bother to invoke the renderer if we're not visible on the
       display */
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
