// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlueMapWindow.hpp"
#include "Input/InputEvents.hpp"
#include "Screen/Layout.hpp"
#include "Simulator.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Math/FastMath.hpp"
#include "util/Compiler.h"
#include "Interface.hpp"
#include "Pan.hpp"
#include "Topography/Thread.hpp"
#include "Asset.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Geo/GeoVector.hpp"

#ifdef USE_X11
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#ifdef ENABLE_SDL
#include <SDL_keyboard.h>
#endif

#include <algorithm> // for std::clamp()

void
GlueMapWindow::OnCreate()
{
  MapWindow::OnCreate();

  visible_projection.SetScale(CommonInterface::GetMapSettings().cruise_scale);
}

void
GlueMapWindow::OnDestroy() noexcept
{
  /* stop the TopographyThread and the TerrainThread */
  SetTopography(nullptr);
  SetTerrain(nullptr);

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  map_item_timer.Cancel();

  MapWindow::OnDestroy();
}

bool
GlueMapWindow::OnMouseDouble([[maybe_unused]] PixelPoint p) noexcept
{
  map_item_timer.Cancel();

  mouse_down_clock.Update();

  InputEvents::ShowMenu();
  ignore_single_click = true;
  return true;
}

bool
GlueMapWindow::OnMouseMove(PixelPoint p, unsigned keys) noexcept
{
  /* allow a bigger threshold on touch screens */
  const unsigned threshold = Layout::Scale(HasTouchScreen() ? 50 : 10);
  if (drag_mode != DRAG_NONE && arm_mapitem_list &&
      ((unsigned)ManhattanDistance(drag_start, p) > threshold ||
       mouse_down_clock.Elapsed() > std::chrono::milliseconds(200)))
    arm_mapitem_list = false;

  switch (drag_mode) {
  case DRAG_NONE:
    break;

#ifdef HAVE_MULTI_TOUCH
  case DRAG_MULTI_TOUCH_PAN:
#endif
  case DRAG_PAN:
    SetLocation(drag_projection.GetGeoLocation()
                + drag_start_geopoint
                - drag_projection.ScreenToGeo(p));
    QuickRedraw();

#ifdef ENABLE_OPENGL
    kinetic_x.MouseMove(p.x);
    kinetic_y.MouseMove(p.y);
#endif
    return true;

  case DRAG_GESTURE:
    gestures.Update(p);

    /* invoke PaintWindow's Invalidate() implementation instead of
       DoubleBufferWindow's in order to reuse the buffered map */
    PaintWindow::Invalidate();
    return true;

  case DRAG_SIMULATOR:
    return true;
  }

  return MapWindow::OnMouseMove(p, keys);
}

[[gnu::pure]]
static bool
IsCtrlKeyPressed() noexcept
{
#ifdef ENABLE_SDL
  return SDL_GetModState() & (KMOD_LCTRL|KMOD_RCTRL);
#elif defined(USE_WINUSER)
  return GetKeyState(VK_CONTROL) & 0x8000;
#elif defined(USE_X11)
  return UI::event_queue->WasCtrlClick();
#else
  return false;
#endif
}

bool
GlueMapWindow::OnMouseDown(PixelPoint p) noexcept
{
  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  // Ignore single click event if double click detected
  if (ignore_single_click || drag_mode != DRAG_NONE)
    return true;

  if (is_simulator() && IsCtrlKeyPressed() && visible_projection.IsValid()) {
    /* clicking with Ctrl key held moves the simulator to the click
       location instantly */
    const GeoPoint location = visible_projection.ScreenToGeo(p);
    backend_components->device_blackboard->SetSimulatorLocation(location);
    return true;
  }

  mouse_down_clock.Update();

  const bool had_focus = HasFocus();
  SetFocus();

  drag_start = p;

  if (!visible_projection.IsValid()) {
    gestures.Start(p, Layout::Scale(20));
    drag_mode = DRAG_GESTURE;
    SetCapture();
    return true;
  }

  drag_start_geopoint = visible_projection.ScreenToGeo(p);
  arm_mapitem_list = had_focus;

  switch (follow_mode) {
  case FOLLOW_SELF:
    break;

  case FOLLOW_PAN:
    drag_mode = DRAG_PAN;
    drag_projection = visible_projection;

#ifdef ENABLE_OPENGL
    kinetic_x.MouseDown(p.x);
    kinetic_y.MouseDown(p.y);
#endif

    break;
  }

  if (CommonInterface::Basic().gps.simulator && drag_mode == DRAG_NONE)
    if (compare_squared(visible_projection.GetScreenOrigin().x - p.x,
                        visible_projection.GetScreenOrigin().y - p.y,
                        Layout::Scale(30)) != 1)
        drag_mode = DRAG_SIMULATOR;
  if (drag_mode == DRAG_NONE ) {
    gestures.Start(p, Layout::Scale(20));
    drag_mode = DRAG_GESTURE;
  }

  if (drag_mode != DRAG_NONE)
    SetCapture();

  return true;
}

bool
GlueMapWindow::OnMouseUp(PixelPoint p) noexcept
{
  if (drag_mode != DRAG_NONE)
    ReleaseCapture();

  // Ignore single click event if double click detected
  if (ignore_single_click) {
    ignore_single_click = false;
    return true;
  }

  const auto click_time = mouse_down_clock.Elapsed();
  mouse_down_clock.Reset();

  DragMode old_drag_mode = drag_mode;
  drag_mode = DRAG_NONE;

  switch (old_drag_mode) {
  case DRAG_NONE:
    /* skip the arm_mapitem_list check below */
    return false;

#ifdef HAVE_MULTI_TOUCH
  case DRAG_MULTI_TOUCH_PAN:
    follow_mode = FOLLOW_SELF;
    ::PanTo(visible_projection.GetGeoScreenCenter());
    return true;
#endif

  case DRAG_PAN:
#ifndef ENABLE_OPENGL
    /* allow the use of the stretched last buffer for the next two
       redraws */
    scale_buffer = 2;
#endif

#ifdef ENABLE_OPENGL
    kinetic_x.MouseUp(p.x);
    kinetic_y.MouseUp(p.y);
    kinetic_timer.Schedule(std::chrono::milliseconds(30));
#endif
    break;

  case DRAG_SIMULATOR:
    if (click_time > std::chrono::milliseconds(50) &&
        compare_squared(drag_start.x - p.x, drag_start.y - p.y,
                        Layout::Scale(36)) == 1) {
      GeoPoint location = visible_projection.ScreenToGeo(p);

      double distance = hypot(drag_start.x - p.x, drag_start.y - p.y);

      // This drag moves the aircraft (changes speed and direction)
      const Angle old_bearing = CommonInterface::Basic().track;
      const auto min_speed = 1.1 *
        CommonInterface::GetComputerSettings().polar.glide_polar_task.GetVMin();
      const Angle new_bearing = drag_start_geopoint.Bearing(location);

      auto &device_blackboard = *backend_components->device_blackboard;
      
      if ((new_bearing - old_bearing).AsDelta().Absolute() < Angle::Degrees(30) ||
          (CommonInterface::Basic().ground_speed < min_speed))
        device_blackboard.SetSpeed(std::clamp(distance / Layout::FastScale(3),
                                              min_speed, 100.));

      device_blackboard.SetTrack(new_bearing);
      // change bearing without changing speed if direction change > 30
      // 20080815 JMW prevent dragging to stop glider

      return true;
    }

    break;

  case DRAG_GESTURE:
    const TCHAR* gesture = gestures.Finish();
    if (gesture && OnMouseGesture(gesture))
      return true;

    break;
  }

  if (arm_mapitem_list) {
    map_item_timer.Schedule(std::chrono::milliseconds(200));
    return true;
  }

  return false;
}

bool
GlueMapWindow::OnMouseWheel([[maybe_unused]] PixelPoint p,
                            [[maybe_unused]] int delta) noexcept
{
  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  if (drag_mode != DRAG_NONE)
    return true;

  if (delta > 0)
    // zoom in
    InputEvents::sub_ScaleZoom(1);
  else if (delta < 0)
    // zoom out
    InputEvents::sub_ScaleZoom(-1);

  return true;
}

#ifdef HAVE_MULTI_TOUCH

bool
GlueMapWindow::OnMultiTouchDown() noexcept
{
  if (!visible_projection.IsValid())
    return false;

  if (drag_mode == DRAG_GESTURE)
    gestures.Finish();
  else if (follow_mode != FOLLOW_SELF)
    return false;

  /* start panning on MultiTouch event */

  drag_mode = DRAG_MULTI_TOUCH_PAN;
  drag_projection = visible_projection;
  follow_mode = FOLLOW_PAN;
  return true;
}

#endif /* HAVE_MULTI_TOUCH */

bool
GlueMapWindow::OnMouseGesture(const TCHAR *gesture) noexcept
{
  return InputEvents::processGesture(gesture);
}

bool
GlueMapWindow::OnKeyDown(unsigned key_code) noexcept
{
  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  if (InputEvents::processKey(key_code)) {
    return true; // don't go to default handler
  }

  return false;
}

void
GlueMapWindow::OnCancelMode() noexcept
{
  MapWindow::OnCancelMode();

  if (drag_mode != DRAG_NONE) {
#ifdef HAVE_MULTI_TOUCH
    if (drag_mode == DRAG_MULTI_TOUCH_PAN)
      follow_mode = FOLLOW_SELF;
#endif

    if (drag_mode == DRAG_GESTURE)
      gestures.Finish();

    ReleaseCapture();
    drag_mode = DRAG_NONE;
  }

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
#endif

  map_item_timer.Cancel();
}

void
GlueMapWindow::OnPaint(Canvas &canvas) noexcept
{
  MapWindow::OnPaint(canvas);

  // Draw center screen cross hair in pan mode
  if (IsPanning())
    DrawCrossHairs(canvas);

  DrawGesture(canvas);
}

void
GlueMapWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
  ExchangeBlackboard();

  EnterDrawThread();
#endif

  MapWindow::OnPaintBuffer(canvas);

  DrawMapScale(canvas, GetClientRect(), render_projection);
  if (IsPanning())
    DrawPanInfo(canvas);

#ifdef ENABLE_OPENGL
  LeaveDrawThread();
#endif
}

void
GlueMapWindow::OnMapItemTimer() noexcept
{
  if (!InputEvents::IsDefault() && !IsPanning()) {
    InputEvents::HideMenu();
    return;
  }

  ShowMapItems(drag_start_geopoint, false);
}

void
GlueMapWindow::OnSmoothUpdateTimer() noexcept
{
  // Use CommonInterface::Basic() since we're in the main thread, not DrawThread
  const NMEAInfo &basic = CommonInterface::Basic();
  const DerivedInfo &calculated = CommonInterface::Calculated();
  const bool circling =
    CommonInterface::GetUIState().display_mode == DisplayMode::CIRCLING;
  
  // Only update map center if we're following the aircraft (not panning)
  if (IsPanning() || !IsNearSelf()) {
    smooth_update_timer.Cancel();
    return;
  }

  // Get current map center location
  if (!visible_projection.IsValid()) {
    smooth_update_timer.Cancel();
    return;
  }
  
  const GeoPoint current_map_center = visible_projection.GetGeoLocation();
  GeoPoint target_location;
  bool has_target = false;
  
  // In circling mode, smoothly follow thermal center if available
  if (circling && calculated.thermal_locator.estimate_valid && 
      has_last_thermal_center && last_thermal_center.IsValid() &&
      basic.location_available) {
    // Use the stored thermal center (updated at 1 Hz) for smooth interpolation
    // Calculate target map center (interpolated between aircraft and thermal center)
    const auto d_t = last_thermal_center.DistanceS(basic.location);
    if (d_t > 0) {
      const auto d_max = 2 * visible_projection.GetMapScale();
      const auto t = std::min(d_t, d_max) / d_t;
      target_location = basic.location.Interpolate(last_thermal_center, t);
      has_target = true;
    } else {
      // Thermal center is at aircraft location
      target_location = basic.location;
      has_target = true;
    }
  }
  
  // If not in circling mode or thermal center not available, use aircraft position dead reckoning
  if (!has_target) {
    // Only update if we have stored GPS location and aircraft is moving
    if (!has_last_gps_location || !last_gps_location_time.IsDefined() ||
        !basic.location_available || !basic.ground_speed_available ||
        basic.ground_speed <= 0.5) {
      // Stop timer if GPS is lost or aircraft stopped
      smooth_update_timer.Cancel();
      return;
    }
    
    // Calculate predicted aircraft position using dead reckoning
    // Use wall-clock time for accurate elapsed time calculation
    const auto elapsed_wall = last_gps_fix_clock.Elapsed();
    // Elapsed() can return negative if clock was never updated
    GeoPoint predicted_location = last_gps_location;
    
    if (elapsed_wall.count() >= 0) {
      const double elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(elapsed_wall).count();
      
      // Use dead reckoning if we have valid speed and track
      // Angle is always valid, just check if speed is positive
      if (elapsed_seconds > 0 && elapsed_seconds < 2.0 && 
          last_gps_speed > 0) {
        // Calculate distance traveled since last GPS fix
        const double distance = last_gps_speed * elapsed_seconds;
        
        // Calculate predicted location using GeoVector
        const GeoVector vector(distance, last_gps_track);
        predicted_location = vector.EndPoint(last_gps_location);
      } else if (elapsed_seconds >= 2.0 || !basic.time_available) {
        // Too much time elapsed or no time - use current GPS location
        predicted_location = basic.location;
      }
    } else {
      // Clock not initialized - use current GPS location
      predicted_location = basic.location;
    }
    
    target_location = predicted_location;
    has_target = true;
  }
  
  if (!has_target) {
    smooth_update_timer.Cancel();
    return;
  }
  
  // Calculate distance to target location
  const double distance_to_target = current_map_center.DistanceS(target_location);
  const double distance_pixels = visible_projection.DistanceMetersToPixels(distance_to_target);
  
  // Update map center to follow target smoothly
  // Use a constant blend factor for constant-speed panning
  const double blend_factor = 0.5; // 50% toward target per frame = smooth constant movement
  
  // Only update if the distance is significant (more than 0.5 pixels)
  if (distance_pixels > 0.5) {
    // Interpolate between current map center and target location
    const GeoPoint target = current_map_center.Interpolate(target_location, blend_factor);
    SetLocation(target);
    QuickRedraw();
  } else {
    // Already close enough - just trigger a redraw
    PartialRedraw();
  }
  
  // Reschedule for next update (10 Hz = 100ms)
  smooth_update_timer.Schedule(std::chrono::milliseconds(100));
}

#ifdef ENABLE_OPENGL

void
GlueMapWindow::OnKineticTimer() noexcept
{
  if (kinetic_x.IsSteady() && kinetic_y.IsSteady()) {
    kinetic_timer.Cancel();
    return;
  }

  auto location = drag_projection.ScreenToGeo({kinetic_x.GetPosition(), kinetic_y.GetPosition()});
  location = drag_projection.GetGeoLocation() +
    drag_start_geopoint - location;

  SetLocation(location);
  QuickRedraw();
}

#endif

void
GlueMapWindow::Render(Canvas &canvas, const PixelRect &rc) noexcept
{
  MapWindow::Render(canvas, rc);

  if (IsNearSelf()) {
    draw_sw.Mark("DrawGlueMisc");
    if (GetMapSettings().show_thermal_profile)
      DrawThermalBand(canvas, rc);
    DrawStallRatio(canvas, rc);
    DrawFlightMode(canvas, rc);
    DrawFinalGlide(canvas, rc);
    DrawVario(canvas, rc);
    DrawGPSStatus(canvas, rc, Basic());
  }
}
