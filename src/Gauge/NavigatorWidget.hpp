// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Blackboard/BlackboardListener.hpp"
#include "Interface.hpp"
#include "Look/Look.hpp"
#include "Look/NavigatorLook.hpp"
#include "MainWindow.hpp"
#include "Renderer/NavigatorRenderer.hpp"
#include "UIUtil/GestureManager.hpp"
#include "Widget/WindowWidget.hpp"

/**
 * A Window which renders a Navigator
 */
class NavigatorWindow : public PaintWindow {
  const NavigatorLook &look_nav;
  const TaskLook &look_task;
  const InfoBoxLook &look_infobox;

  const NavType nav_type{NavType::NAVIGATOR};
  const bool is_navigator_top_position{};

  NavigatorRenderer renderer;

  AttitudeState attitude;

  GestureManager gestures;
  bool dragging{false};
  bool ignore_single_click{false};

  PeriodClock mouse_down_clock;

public:
  /**
   * Constructor. Initializes most class members.
   */
  NavigatorWindow(const NavigatorLook &_look_nav, const TaskLook &_look_task,
                    const InfoBoxLook &_look_infobox, NavType nav_type_param,
                    bool is_nav_top) noexcept;

  void ReadBlackboard(const AttitudeState &_attitude) noexcept;

protected:
  void OnPaint(Canvas &canvas) noexcept override;

private:
  void StopDragging();

public:
  bool OnMouseGesture(const char *gesture) noexcept;
  bool OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept override;
  bool OnMouseDown(PixelPoint p) noexcept override;
  bool OnMouseUp([[maybe_unused]] PixelPoint p) noexcept override;
  bool OnMouseMove(PixelPoint p,
                   [[maybe_unused]] unsigned keys) noexcept override;
  void OnCancelMode() noexcept override;
  bool OnKeyDown(unsigned key_code) noexcept override;
};

class NavigatorWidget final : public NullWidget,
                              private NullBlackboardListener {
  std::unique_ptr<NavigatorWindow> navigator_window;

  NavType nav_type{NavType::NAVIGATOR};
  bool is_navigator_top_position{true};

public:
  explicit NavigatorWidget(NavType type, bool is_nav_top = true) noexcept
      : nav_type(type), is_navigator_top_position(is_nav_top) {}

  ~NavigatorWidget() noexcept = default;

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Move(const PixelRect &rc) noexcept override;
  bool SetFocus() noexcept override;

  PixelSize GetMinimumSize() const noexcept override;

  NavigatorWindow *GetWindow() const noexcept
  {
    return navigator_window.get();
  }

private:
  void Update(const MoreData &basic) noexcept;
  void UpdateLayout() noexcept;

  /* virtual methods from class BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) noexcept override;
};
