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

#ifndef XCSOAR_MANAGED_WIDGET_HPP
#define XCSOAR_MANAGED_WIDGET_HPP

#include "ui/dim/Rect.hpp"

#include <cstdint>
#include <memory>

class ContainerWindow;
class Widget;

/**
 * A wrapper for a #Widget pointer that keeps track of the Widget's
 * state.  It will enforce some of the #Widget API rules.  It will
 * take care for hiding, unpreparing and deleting the given #Widget
 * instance, and will not call Widget::Prepare() and Widget::Show()
 * twice.
 */
class ManagedWidget {
  ContainerWindow *parent = nullptr;
  PixelRect position;

  Widget *widget = nullptr;

  /**
   * Only valid if the #widget is set.
   */
  enum class State : uint8_t {
    NONE,
    INITIALISED,
    PREPARED,
    VISIBLE,
  } state;

#ifndef NDEBUG
  bool have_position = false;
#endif

public:
  ManagedWidget() noexcept = default;

  explicit ManagedWidget(ContainerWindow &_parent) noexcept
    :parent(&_parent) {}

  ManagedWidget(ContainerWindow &_parent, Widget *_widget) noexcept
    :parent(&_parent), widget(_widget), state(State::NONE) {}

  explicit ManagedWidget(Widget *_widget) noexcept
    :widget(_widget), state(State::NONE) {}

  ~ManagedWidget() noexcept {
    Clear();
  }

  ManagedWidget(const ManagedWidget &) = delete;
  ManagedWidget &operator=(const ManagedWidget &) = delete;

  /**
   * Return the Widget object (in the current state), and "forget"
   * about it.
   */
  Widget *Steal() noexcept {
    Widget *result = widget;
    widget = nullptr;
    return result;
  }

  bool IsDefined() const noexcept {
    return widget != nullptr;
  }

  bool IsPrepared() const noexcept {
    return IsDefined() && state >= State::PREPARED;
  }

  bool IsVisible() const noexcept {
    return IsDefined() && state == State::VISIBLE;
  }

  /**
   * This call is only needed (and allowed) if no parent was passed to
   * the constructor.
   */
  void Initialise(ContainerWindow &_parent, const PixelRect &_position);

  /**
   * Ensure that the Widget is prepared.
   */
  void Prepare();

  void Unprepare() noexcept;
  void Clear() noexcept;

  /**
   * @param widget an uninitialised Widget
   */
  void Set(Widget *widget) noexcept;
  void Set(std::unique_ptr<Widget> widget) noexcept;

  Widget *Get() noexcept {
    return widget;
  }

  void Move(const PixelRect &position) noexcept;

  void Show() noexcept;
  void Hide() noexcept;

  void SetVisible(bool _visible) noexcept;

  bool Save(bool &changed);
  bool SetFocus() noexcept;

  [[gnu::pure]]
  bool HasFocus() const noexcept;

  bool KeyPress(unsigned key_code) noexcept;
};

#endif
