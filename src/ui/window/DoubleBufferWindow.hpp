// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef ENABLE_OPENGL

#include "BufferWindow.hpp"

/* there's no DrawThread on OpenGL, so this is just a normal
   BufferWindow */
using DoubleBufferWindow = BufferWindow;

#else

#include "PaintWindow.hpp"
#include "ui/canvas/BufferCanvas.hpp"
#include "ui/event/Notify.hpp"
#include "thread/Mutex.hxx"

/**
 * A #PaintWindow with double buffered painting, to avoid flickering.
 * Having two buffers, one thread may render, while the main threadd
 * copies the other buffer to the screen.
 */
class DoubleBufferWindow : public PaintWindow {
  UI::Notify invalidate_notify{[this]{ Invalidate(); }};

  BufferCanvas buffers[2];

  /**
   * The buffer currently drawn into.  This buffer may only be
   * accessed by the drawing thread.  The other buffer (current^1) may
   * only be accessed by the main thread.
   *
   * Protected by #mutex.
   */
  unsigned current = 0;

  /**
   * The window size for the next OnPaintBuffer() call.  This field is
   * thread-safe, unlike Window::GetSize().
   *
   * Protected by #mutex.
   */
  PixelSize next_size;

protected:
  /**
   * This mutex protects the fields #current and #next_size.
   */
  mutable Mutex mutex;

private:
  /**
   * Returns the Canvas which is currently used for rendering.  This
   * method may only be called within the drawing thread.
   */
  auto &GetPaintCanvas() noexcept {
    return buffers[current];
  }

protected:
  /**
   * Returns the Canvas which is currently visible.  A call to this
   * method must be protected with the Mutex.
   */
  const Canvas &GetVisibleCanvas() const noexcept {
    return buffers[current ^ 1];
  }

protected:
  /* virtual methods from class Window */
  void OnCreate() override;
  void OnDestroy() noexcept override;
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;

  /**
   * Paint into the given #Canvas.  This is called from the thread
   * that called Repaint().  The caller holds the mutex lock, and this
   * method may (temporarily) unlock it for expensive drawing
   * operations.
   */
  virtual void OnPaintBuffer(Canvas &canvas) noexcept = 0;

public:
  /**
   * Repaint via virtual method OnPaintBuffer() into the current
   * buffer and flip it.
   *
   * This method is thread-safe.
   */
  void Repaint() noexcept;
};

#endif
