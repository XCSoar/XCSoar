// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#ifdef WIN32_LEAN_AND_MEAN
#include "ui/canvas/gdi/Canvas.hpp"
#endif
#include "NavigatorWidget.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/TaskManager.hpp"
#include "InfoBoxes/InfoBoxWindow.hpp"
#include "Input/InputEvents.hpp"
#include "Renderer/NavigatorRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIGlobals.hpp"

NavigatorWindow::NavigatorWindow(const NavigatorLook &_look_nav,
                                 const TaskLook &_look_task,
                                 const InfoBoxLook &_look_infobox) noexcept
    : look_nav(_look_nav),
      look_task(_look_task),
      look_infobox(_look_infobox),
      dragging(false)
{
}

void
NavigatorWindow::ReadBlackboard(const AttitudeState &_attitude) noexcept
{
  attitude = _attitude;
}

void
NavigatorWindow::OnPaint(Canvas &canvas) noexcept
{
  if (look_nav.inverse)
    canvas.Clear(COLOR_BLACK);
  else
    canvas.ClearWhite();

  TaskType tp{TaskType::NONE};
  WaypointPtr wp_before;
  WaypointPtr wp_current;

  unsigned task_size{};

  auto *protected_task_manager =
      backend_components->protected_task_manager.get();
  if (protected_task_manager != nullptr) {
    ProtectedTaskManager::Lease lease(*protected_task_manager);
    const auto &stats = lease->GetStats();

    tp = lease->GetMode();

    if (stats.task_valid) {
      if (tp == TaskType::ORDERED) {

        const OrderedTask &task = lease->GetOrderedTask();

        task_size = task.TaskSize();

        wp_current = task.GetActiveTaskPoint()->GetWaypointPtr();

        auto i = task.GetActiveIndex();
        if (i == 0) wp_before = nullptr;
        else wp_before = task.GetPoint(i - 1).GetWaypointPtr();
      } else {
        wp_current =
            lease->GetActiveTask()->GetActiveTaskPoint()->GetWaypointPtr();
        wp_before = nullptr;
      }
    } else {
      wp_current = nullptr;
      wp_before = nullptr;
    }
  }

  const PixelRect frame_navigator =
      canvas.GetRect().WithPadding(Layout::Scale(1));

  const int fnw_height = canvas.GetHeight();
  const int fnw_width = canvas.GetWidth();

  PixelPoint pt_origin{fnw_width * 18 / 100, fnw_height * 1 / 10};
  PixelSize frame_size{fnw_width * 8 / 10, fnw_height * 55 / 100};
  PixelRect frame_navigator_waypoint{pt_origin, frame_size};

  NavigatorRenderer::DrawFrame(canvas, frame_navigator, look_nav);
  NavigatorRenderer::DrawFrame(canvas, frame_navigator_waypoint, look_nav);

  if (tp == TaskType::ORDERED)
    NavigatorRenderer::DrawProgressTask(
        CommonInterface::Calculated().common_stats.ordered_summary, canvas,
        canvas.GetRect(), look_nav, look_task);

  NavigatorRenderer::DrawWaypointsIconsTitle(canvas, wp_before, wp_current,
                                             task_size, look_nav);
}

void
NavigatorWindow::StopDragging()
{
  if (!dragging) return;

  dragging = false;
  ReleaseCapture();
}

bool
NavigatorWindow::OnMouseGesture(const TCHAR *gesture) noexcept
{
  if (StringIsEqual(gesture, _T("U"))) {
    InputEvents::ShowMenu();
    return true;
  }
  if (StringIsEqual(gesture, _T("D"))) {
    InputEvents::ShowMenu();
    return true;
  }
  if (StringIsEqual(gesture, _T("R"))) {
    InputEvents::eventAdjustWaypoint(_T("previouswrap"));
    return true;
  }
  if (StringIsEqual(gesture, _T("L"))) {
    InputEvents::eventAdjustWaypoint(_T("nextwrap"));
    return true;
  }
  if (StringIsEqual(gesture, _T("UD"))) {
    InputEvents::ShowMenu();
    return true;
  }
  if (StringIsEqual(gesture, _T("DR"))) {
    InputEvents::ShowMenu();
    return true;
  }
  if (StringIsEqual(gesture, _T("RL"))) {
    InputEvents::eventSetup(_T("Target"));
    return true;
  }
  if (StringIsEqual(gesture, _T("LR"))) {
    InputEvents::eventSetup(_T("Alternates"));
    return true;
  }
  if (gesture) {
    InputEvents::ShowMenu();
    return true;
  }

  return InputEvents::processGesture(gesture);
}

bool
NavigatorWindow::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  StopDragging();
  ignore_single_click = true;
  InputEvents::ShowMenu();
  return true;
}

bool
NavigatorWindow::OnMouseDown(PixelPoint p) noexcept
{
  // Ignore single click event if double click detected
  if (ignore_single_click) return true;

  mouse_down_clock.Update();

  if (!dragging) {
    dragging = true;
    SetCapture();
    gestures.Start(p, Layout::Scale(20));
  }

  return true;
}

bool
NavigatorWindow::OnMouseUp([[maybe_unused]] PixelPoint p) noexcept
{
  // Ignore single click event if double click detected
  if (ignore_single_click) {
    ignore_single_click = false;
    return true;
  }

  const auto click_time = mouse_down_clock.Elapsed();
  mouse_down_clock.Reset();

  if (dragging) {
    StopDragging();

    const TCHAR *gesture = gestures.Finish();
    if (gesture && OnMouseGesture(gesture)) return true;

    if (click_time > std::chrono::milliseconds(400) &&
        click_time < std::chrono::milliseconds(1000))
      // on gesture mouse up == simpleClick
      InputEvents::eventAnalysis(_T("AnalysisPage::TASK"));

    else if (click_time > std::chrono::milliseconds(1000) &&
             click_time < std::chrono::milliseconds(3000))
      InputEvents::eventSetup(_T("Task"));

    else return false;
  }

  return false;
}

bool
NavigatorWindow::OnMouseMove(PixelPoint p,
                             [[maybe_unused]] unsigned keys) noexcept
{
  if (dragging) gestures.Update(p);

  return true;
}

void
NavigatorWindow::OnCancelMode() noexcept
{
#ifndef USE_WINUSER
  ReleaseCapture();
#endif
  StopDragging();
}

bool
NavigatorWindow::OnKeyDown(unsigned key_code) noexcept
{
  return InputEvents::processKey(key_code);
}

void
NavigatorWidget::Update([[maybe_unused]] const MoreData &basic) noexcept
{
  navigator_window->ReadBlackboard(basic.attitude);
}

void
NavigatorWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const Look &look = UIGlobals::GetLook();

  WindowStyle style;
  style.Hide();
  style.Disable();

  navigator_window = std::make_unique<NavigatorWindow>(
      look.navigator,
      look.map.task, look.info_box);
  navigator_window->Create(parent, rc, style);
  navigator_window->Move(rc);
}

void
NavigatorWidget::Show([[maybe_unused]] const PixelRect &rc) noexcept
{
  Update(CommonInterface::Basic());
  CommonInterface::GetLiveBlackboard().AddListener(*this);
  navigator_window->Show();
}

void
NavigatorWidget::Hide() noexcept
{
  navigator_window->Hide();
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
}

void
NavigatorWidget::Move(const PixelRect &rc) noexcept
{
  navigator_window->Move(rc);
}

bool
NavigatorWidget::SetFocus() noexcept
{
  return false;
}

PixelSize
NavigatorWidget::GetMinimumSize() const noexcept
{
  return PixelSize{
      CommonInterface::GetUISettings().navigator.navigator_height};
}

void
NavigatorWidget::OnGPSUpdate(const MoreData &basic) noexcept
{
  Update(basic);
}

void
NavigatorWidget::UpdateLayout() noexcept
{
  const PixelRect rc = navigator_window->GetClientRect();
  navigator_window->Move(rc);
}
