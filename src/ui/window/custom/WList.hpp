// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "../Window.hpp"

#include <boost/intrusive/list.hpp>

#include <cassert>

struct PixelPoint;
class Window;
class Canvas;

/**
 * A container for more #Window objects.  It is used by the SDL/OpenGL
 * #ContainerWindow implementation to manage its children.
 */
class WindowList {
  typedef boost::intrusive::list<Window,
                                 boost::intrusive::member_hook<Window,
                                                               Window::SiblingsHook,
                                                               &Window::siblings>,
                                 boost::intrusive::constant_time_size<false>> List;

  List list;

public:
  ~WindowList() noexcept {
    assert(list.empty());
  }

public:
  void Add(Window &w) noexcept {
    assert(!Contains(w));

    list.push_back(w);
  }

  void Remove(Window &w) noexcept {
    assert(Contains(w));

    list.erase(list.iterator_to(w));
  }

  /**
   * Remove and destroy all contained windows.
   */
  void Clear() noexcept;

  [[gnu::pure]]
  bool Contains(const Window &w) const noexcept;

  /**
   * Is this window covered by a sibling?
   */
  [[gnu::pure]]
  bool IsCovered(const Window &w) const noexcept;

  void BringToTop(Window &w) noexcept;
  void BringToBottom(Window &w) noexcept;

  /**
   * Locate a window by its relative coordinates.
   */
  [[gnu::pure]]
  Window *FindAt(PixelPoint p) noexcept;

  [[gnu::pure]]
  static Window *FindControl(List::iterator i, List::iterator end) noexcept;

  [[gnu::pure]]
  static Window *FindControl(List::reverse_iterator i,
                             List::reverse_iterator end) noexcept;

  [[gnu::pure]]
  Window *FindFirstControl() noexcept;

  [[gnu::pure]]
  Window *FindLastControl() noexcept;

  [[gnu::pure]]
  Window *FindNextChildControl(Window *reference) noexcept;

  [[gnu::pure]]
  Window *FindPreviousChildControl(Window *reference) noexcept;

  void Paint(Canvas &canvas) noexcept;
};
