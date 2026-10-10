// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * Mouse dispatch in a #ContainerWindow: which child gets a press,
 * and what a disabled child does with one (#512).
 */

#include "ui/window/ContainerWindow.hpp"
#include "Screen/Debug.hpp"
#include "TestUtil.hpp"

#if defined(ENABLE_OPENGL) && !defined(NDEBUG)
#include "ui/canvas/opengl/Debug.hpp"
#endif

/**
 * A child window which counts the mouse events it receives and
 * answers them with a fixed result, like a control that handles a
 * press (true) or a passive gauge that does not (false).
 */
class ProbeWindow final : public PaintWindow {
  const bool handles;

public:
  unsigned downs = 0, ups = 0, doubles = 0;

  explicit ProbeWindow(bool _handles) noexcept
    :handles(_handles) {}

  void Reset() noexcept {
    downs = ups = doubles = 0;
  }

  unsigned Total() const noexcept {
    return downs + ups + doubles;
  }

protected:
  void OnPaint(Canvas &) noexcept override {}

  bool OnMouseDown(PixelPoint) noexcept override {
    ++downs;
    return handles;
  }

  bool OnMouseUp(PixelPoint) noexcept override {
    ++ups;
    return handles;
  }

  bool OnMouseDouble(PixelPoint) noexcept override {
    ++doubles;
    return handles;
  }
};

/**
 * Send a tap and a double tap to @p w, the way the event loop does,
 * and return whether any of them was handled.
 */
static bool
DoubleTap(Window &w, PixelPoint p) noexcept
{
  bool handled = w.OnMouseDown(p);
  handled = w.OnMouseUp(p) || handled;
  return w.OnMouseDouble(p) || handled;
}

/**
 * The layout of the FLARM radar: buttons on top of a window that
 * fills the whole area and opens the menu on a double tap.
 */
static void
TestButtonOverWindow()
{
  ContainerWindow root;
  root.Create(nullptr, {0, 0, 200, 200}, WindowStyle{});

  /* the first child is the topmost one */
  ProbeWindow button{true};
  button.Create(root, {10, 10, 60, 40}, WindowStyle{});

  WindowStyle hidden;
  hidden.Hide();
  ProbeWindow hidden_button{true};
  hidden_button.Create(root, {100, 10, 150, 40}, hidden);

  ProbeWindow view{true};
  view.Create(root, {0, 0, 200, 200}, WindowStyle{});

  const PixelPoint on_button{30, 20};
  const PixelPoint on_hidden{120, 20};
  const PixelPoint elsewhere{100, 150};

  ok1(root.ChildAt(on_button) == &button);
  ok1(root.ChildAt(elsewhere) == &view);
  /* a hidden window covers nothing */
  ok1(root.ChildAt(on_hidden) == &view);

  /* an enabled button takes the double tap */
  ok1(DoubleTap(root, on_button));
  ok1(button.downs == 1 && button.ups == 1 && button.doubles == 1);
  ok1(view.Total() == 0);

  /* a disabled button still covers the window below it: the double
     tap is swallowed and must not reach the radar, which would open
     the menu */
  button.Reset();
  button.SetEnabled(false);
  ok1(root.ChildAt(on_button) == &button);
  ok1(DoubleTap(root, on_button));
  ok1(button.Total() == 0);
  ok1(view.Total() == 0);

  /* beside the button the window below still gets every event */
  view.Reset();
  ok1(DoubleTap(root, elsewhere));
  ok1(view.downs == 1 && view.ups == 1 && view.doubles == 1);

  /* and again once the button is enabled */
  view.Reset();
  button.SetEnabled(true);
  ok1(DoubleTap(root, on_button));
  ok1(button.doubles == 1);
  ok1(view.Total() == 0);
}

/**
 * A window which does not handle a press, such as the vario gauge,
 * leaves it to its parent: the main window opens the menu on a
 * double tap that no child handled.
 */
static void
TestPassiveWindow()
{
  ContainerWindow root;
  root.Create(nullptr, {0, 0, 200, 200}, WindowStyle{});

  ProbeWindow gauge{false};
  gauge.Create(root, {0, 0, 50, 200}, WindowStyle{});

  const PixelPoint on_gauge{20, 100};
  ok1(!DoubleTap(root, on_gauge));
  ok1(gauge.doubles == 1);

  /* nothing below at all */
  ok1(root.ChildAt({150, 100}) == nullptr);
  ok1(!DoubleTap(root, {150, 100}));
}

int
main()
{
  ScreenInitialized();

#if defined(ENABLE_OPENGL) && !defined(NDEBUG)
  /* Window::AssertThread() expects the thread which set up OpenGL */
#ifdef _WIN32
  OpenGL::thread = GetCurrentThreadId();
#else
  OpenGL::thread = pthread_self();
#endif
#endif

  plan_tests(19);

  TestButtonOverWindow();
  TestPassiveWindow();

  ScreenDeinitialized();

  return exit_status();
}
