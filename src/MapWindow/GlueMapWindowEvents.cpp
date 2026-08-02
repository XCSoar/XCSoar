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
#include "ActionInterface.hpp"
#include "UserMapScale.hpp"
#ifdef HAVE_EDL
#include "UIState.hpp"
#endif

#ifdef USE_X11
#include "ui/event/Globals.hpp"
#include "ui/event/Queue.hpp"
#endif

#ifdef ENABLE_SDL
#include <SDL_keyboard.h>
#endif

#include <algorithm> // for std::clamp()
#include <cmath> // for std::hypot()

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
  CancelZoomAnimation();
  terrain_quantisation_timer.Cancel();
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
    if (resume_pan_after_pinch) {
      drag_projection = visible_projection;
      drag_start = p;
      drag_start_geopoint = drag_projection.ScreenToGeo(p);
      resume_pan_after_pinch = false;
      return true;
    }

    if (!multi_touch_pan_ui &&
        (unsigned)ManhattanDistance(drag_start, p) > Layout::Scale(20u)) {
      CommitMultiTouchPanUI();

      /* entering pan mode may have resized the map; re-base the drag so
         the map does not jump on the next motion event */
      drag_projection = visible_projection;
      drag_start = p;
      drag_start_geopoint = drag_projection.ScreenToGeo(p);
      return true;
    }

    [[fallthrough]];
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

#ifdef HAVE_MULTI_TOUCH
  case DRAG_MULTI_TOUCH_PINCH:
    /* primary-pointer mouse motion is ignored; scale/pan come from
       OnMultiTouchMove() */
    return true;
#endif

  case DRAG_GESTURE:
    gestures.Update(p);

    /* invoke PaintWindow's Invalidate() implementation instead of
       DoubleBufferWindow's in order to reuse the buffered map */
    PaintWindow::Invalidate();
#ifdef ENABLE_OPENGL
    NoteTerrainQuantisationUserActivity();
#endif
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

  bool was_kinetic_motion = false;
#ifdef ENABLE_OPENGL
  was_kinetic_motion = kinetic_timer.IsActive();
  kinetic_timer.Cancel();
  CancelZoomAnimation();
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
  /* If this touch stopped kinetic motion, allow immediate re-drag, but
     don't arm tap map-item selection from this same touch. */
  arm_mapitem_list = had_focus && !was_kinetic_motion;

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

    /* a multi-touch gesture must still be finished: it has already
       switched to FOLLOW_PAN, and only the code below enters pan mode
       properly, including its menu */
    if (!GestureOwnsMap())
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
  case DRAG_MULTI_TOUCH_PINCH:
    PersistCurrentScale();

    resume_pan_after_pinch = false;

    if (multi_touch_was_panning || multi_touch_pan_ui)
      /* pan UI was already active, or CommitMultiTouchPanUI() ran
         during the gesture; stay in FOLLOW_PAN */
      follow_mode = FOLLOW_PAN;
    else {
      /* a pure pinch-zoom only changed the scale; resume following the
         aircraft */
      follow_mode = FOLLOW_SELF;
      QuickRedraw();
    }

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

#ifdef HAVE_EDL
    if (CommonInterface::GetUIState().weather.edl.session.IsSuspendedForPan())
      ActionInterface::ScheduleSendUIState();
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
    const char* gesture = gestures.Finish();
    if (gesture && OnMouseGesture(gesture))
      return true;

    /* Menu dismissal doesn't need a valid projection;
       dismiss on tap even when there is no GPS fix */
    if (!InputEvents::IsDefault() && !IsPanning()) {
      InputEvents::HideMenu();
      return true;
    }

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
  CancelZoomAnimation();
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

  if (drag_mode == DRAG_MULTI_TOUCH_PAN ||
      drag_mode == DRAG_MULTI_TOUCH_PINCH)
    /* another finger touched down during the gesture; the state below
       was already captured, and re-capturing it now would read the
       FOLLOW_PAN that this very gesture has set as "was already
       panning", which skips entering pan mode on release */
    return true;

  /* Start gesture ownership only.  Do not set FOLLOW_PAN or pan UI
     yet: that is CommitMultiTouchPanUI()'s job after a drag/twist, so
     chrome and fullscreen layout appear together at the final size. */
  BeginMultiTouchOwnership();

  drag_mode = DRAG_MULTI_TOUCH_PAN;
  drag_projection = visible_projection;
  arm_mapitem_list = false;
  return true;
}

void
GlueMapWindow::DiscardPendingFingerGesture() noexcept
{
  if (drag_mode != DRAG_GESTURE)
    return;

  /* discard without firing; repaint to erase the gesture trail */
  gestures.Finish();
  PaintWindow::Invalidate();
}

void
GlueMapWindow::BeginMultiTouchOwnership() noexcept
{
  DiscardPendingFingerGesture();
  multi_touch_was_panning = IsPanning();
  multi_touch_pan_ui = false;
  resume_pan_after_pinch = false;
  pinch_scaling = false;
  pinch_rotating = false;
}

void
GlueMapWindow::ResetMultiTouchSessionState() noexcept
{
  resume_pan_after_pinch = false;
  multi_touch_pan_ui = false;
  multi_touch_was_panning = false;
  pinch_scaling = false;
  pinch_rotating = false;
  manual_rotation = false;
}

void
GlueMapWindow::CommitMultiTouchPanUI() noexcept
{
  if (multi_touch_pan_ui)
    return;

  multi_touch_pan_ui = true;

  if (multi_touch_was_panning)
    /* pan UI is already active */
    return;

  /* follow_mode is still FOLLOW_SELF, so ShowOnlyMap()'s DisablePan()
     is a no-op; PanTo() expands the layout (coalesced) and only then
     sets FOLLOW_PAN — chrome is drawn at the fullscreen centre. */
  ::PanTo(visible_projection.GetGeoScreenCenter());
}

void
GlueMapWindow::RebasePinchAfterLayoutChange(PixelPoint a, PixelPoint b,
                                            double distance,
                                            PixelPoint centroid) noexcept
{
  pinch_start_distance = distance;
  pinch_start_map_scale = visible_projection.GetMapScale();
  pinch_start_centroid = centroid;
  pinch_anchor_geo = visible_projection.ScreenToGeo(centroid);
  pinch_start_finger_angle =
    Angle::Radians(std::atan2(double(b.y - a.y), double(b.x - a.x)));
  pinch_start_screen_angle = visible_projection.GetScreenAngle();
}

bool
GlueMapWindow::OnMultiTouchMove(PixelPoint a, PixelPoint b) noexcept
{
  if (!visible_projection.IsValid())
    return false;

  const double distance = std::hypot(double(a.x - b.x), double(a.y - b.y));
  if (distance < 1)
    return true;

  const PixelPoint centroid{(a.x + b.x) / 2, (a.y + b.y) / 2};

  if (drag_mode != DRAG_MULTI_TOUCH_PINCH) {
    if (drag_mode != DRAG_MULTI_TOUCH_PAN)
      /* OnMultiTouchDown() did not run; record the pan state here */
      BeginMultiTouchOwnership();
    else
      DiscardPendingFingerGesture();

    drag_mode = DRAG_MULTI_TOUCH_PINCH;
    drag_projection = visible_projection;
    arm_mapitem_list = false;
    pinch_start_distance = distance;
    pinch_start_map_scale = visible_projection.GetMapScale();
    pinch_anchor_geo = visible_projection.ScreenToGeo(centroid);
    pinch_start_centroid = centroid;
    pinch_last_a = a;
    pinch_last_b = b;
    pinch_scaling = false;
    pinch_rotating = false;
    pinch_start_finger_angle =
      Angle::Radians(std::atan2(double(b.y - a.y), double(b.x - a.x)));
    pinch_start_screen_angle = visible_projection.GetScreenAngle();
    SetCapture();
    return true;
  }

  if (pinch_start_distance < 1 || !pinch_anchor_geo.IsValid())
    return true;

  if (a == pinch_last_a && b == pinch_last_b)
    /* one motion event per finger arrives, but both positions are read
       from the current device state; skip the duplicate */
    return true;

  pinch_last_a = a;
  pinch_last_b = b;

  if (!multi_touch_pan_ui &&
      (unsigned)ManhattanDistance(pinch_start_centroid, centroid) >
      Layout::Scale(20u)) {
    CommitMultiTouchPanUI();
    RebasePinchAfterLayoutChange(a, b, distance, centroid);
    return true;
  }

  if (!pinch_scaling) {
    /* Require a noticeable change in finger separation before the map
       is rescaled.  Fingers always wobble while panning, and rescaling
       invalidates the cached terrain and topography rendering, which
       is far more expensive than panning. */
    constexpr double dead_zone = 0.1;
    const double ratio = distance / pinch_start_distance;

    if (ratio > 1 + dead_zone || ratio < 1 - dead_zone) {
      /* re-base so that scaling starts from the current scale without
         a jump */
      pinch_scaling = true;
      pinch_start_distance = distance;
      pinch_start_map_scale = visible_projection.GetMapScale();
    }
  }

  if (pinch_scaling) {
    if (!IsPanning())
      DisableAutoZoomForManualScale();

    const double new_scale = ClampUserMapScale(
      pinch_start_map_scale * (pinch_start_distance / distance));

    visible_projection.SetFreeMapScale(new_scale);
    OnProjectionModified();
  }

  /* two-finger twist rotates the map around the pinch centroid */
  const Angle finger_angle =
    Angle::Radians(std::atan2(double(b.y - a.y), double(b.x - a.x)));

  if (!pinch_rotating) {
    /* ignore small twists so an ordinary pinch/pan does not rotate */
    constexpr double rotate_dead_zone_deg = 8;
    if ((finger_angle - pinch_start_finger_angle).AsDelta().Absolute() >
        Angle::Degrees(rotate_dead_zone_deg)) {
      /* re-base so rotation starts from the current angle without a
         jump */
      pinch_rotating = true;
      pinch_start_finger_angle = finger_angle;
      pinch_start_screen_angle = visible_projection.GetScreenAngle();
    }
  }

  if (pinch_rotating) {
    const Angle twist =
      (finger_angle - pinch_start_finger_angle).AsDelta();

    /* screen y grows downward, so a visually clockwise finger twist
       increases the measured angle; subtract it so the map content
       rotates with the fingers */
    manual_rotation_angle = pinch_start_screen_angle - twist;
    manual_rotation = true;

    /* a rotation is a deliberate manipulation; enter pan mode so the
       chosen angle is held */
    if (!multi_touch_pan_ui) {
      CommitMultiTouchPanUI();
      RebasePinchAfterLayoutChange(a, b, distance, centroid);
      pinch_start_finger_angle = finger_angle;
      pinch_start_screen_angle = manual_rotation_angle;
    }

    visible_projection.SetScreenAngle(manual_rotation_angle);
    OnProjectionModified();
  }

  SetLocation(visible_projection.GetGeoLocation() + pinch_anchor_geo
              - visible_projection.ScreenToGeo(centroid));
  QuickRedraw();
  return true;
}

bool
GlueMapWindow::OnMultiTouchUp() noexcept
{
  if (drag_mode != DRAG_MULTI_TOUCH_PINCH)
    return false;

  /* second finger lifted: keep current scale, fall back to single-
     finger pan with the remaining pointer */
  PersistCurrentScale();

  drag_mode = DRAG_MULTI_TOUCH_PAN;
  resume_pan_after_pinch = true;
  return true;
}

#endif /* HAVE_MULTI_TOUCH */

bool
GlueMapWindow::OnMouseGesture(const char *gesture) noexcept
{
  return InputEvents::processGesture(gesture);
}

bool
GlueMapWindow::OnKeyDown(unsigned key_code) noexcept
{
  map_item_timer.Cancel();

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
  CancelZoomAnimation();
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
    const bool was_multi_touch = GestureOwnsMap();
    if (was_multi_touch) {
      PersistCurrentScale();

      /* multi_touch_pan_ui means pan mode has already been entered,
         and its user interface is visible */
      follow_mode = multi_touch_was_panning || multi_touch_pan_ui
        ? FOLLOW_PAN
        : FOLLOW_SELF;
    }

    ResetMultiTouchSessionState();
#endif

    if (drag_mode == DRAG_GESTURE)
      gestures.Finish();

    ReleaseCapture();
    drag_mode = DRAG_NONE;

#ifdef HAVE_MULTI_TOUCH
    if (was_multi_touch)
      /* drop a held twist angle and refresh after the session flags
         were cleared */
      QuickRedraw();
#endif
  }

#ifdef ENABLE_OPENGL
  kinetic_timer.Cancel();
  CancelZoomAnimation();
#endif

  map_item_timer.Cancel();
}

void
GlueMapWindow::OnPaint(Canvas &canvas) noexcept
{
  MapWindow::OnPaint(canvas);

  if (IsPanChromeVisible())
    DrawCrossHairs(canvas);

  DrawGesture(canvas);
}

void
GlueMapWindow::OnPaintBuffer(Canvas &canvas) noexcept
{
#ifdef ENABLE_OPENGL
  ExchangeBlackboard();
  EnterDrawThread();

  /* PartialRedraw (vario/GPS/topography) invalidates without running
     FullRedraw.  Without a projection update here, Render() draws the
     new aircraft against a stale map origin — the plane slides, then
     the next FullRedraw recenters and the terrain jerks. */
  if (IsNearSelf()) {
    UpdateDisplayMode();
    UpdateScreenAngle();
    UpdateProjection();
    /* Pin origin to the DeviceBlackboard sample we just exchanged so
       aircraft and terrain share one geo origin in this frame. */
    if (Basic().location_available)
      SetLocationLazy(Basic().location);
    UpdateMapScale();
    /* Refresh cached screen_bounds only — not GlueMapWindow's
       UpdateScreenBounds(), which would re-trigger terrain/topo
       threads.  Stale bounds break airspace GeoClip (MapCanvas). */
    MapWindow::UpdateScreenBounds();
  }
#endif

  MapWindow::OnPaintBuffer(canvas);

  DrawMapScale(canvas, GetClientRect(), render_projection);
  if (IsPanChromeVisible())
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
