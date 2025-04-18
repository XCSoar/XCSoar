// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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
