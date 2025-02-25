// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once


#include "Engine/Waypoint/Ptr.hpp"
#include "Engine/Task/TaskType.hpp"
#include "Look/InfoBoxLook.hpp"

struct PixelRect;
struct PixelPoint;
struct NavigatorLook;
struct InfoBoxLook;
struct AttitudeState;
class Canvas;
class TextRenderer;
class WaypointIconRenderer;
struct TaskLook;
struct TaskSummary;


namespace NavigatorRenderer {
/**
* This function is used to create the frame of the Navigator and
* also the frame of the waypoint
*/
void
DrawFrame(Canvas &canvas, const PixelRect &rc, const NavigatorLook &look_nav) noexcept;
}
