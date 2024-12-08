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

/**
* Draw information texts about the current task (ordered task) or 
* the current target (unordered task) 
* e.g. waypoint distance, start time, planned duration time, ...
*/
void
DrawTaskText(Canvas &canvas, TaskType tp, const Waypoint &wp_current, const PixelRect &rc,
         const NavigatorLook &look_nav, const InfoBoxLook &iblook) noexcept;

/**
* Draw the progress of the current task with presntation of each taskpoint
*/
void
DrawProgressTask(const TaskSummary &summary, Canvas &canvas,
                 const PixelRect &rc, const NavigatorLook &look_nav,
                 const TaskLook &look_task) noexcept;

/**
* Draw the icon of the current task and of the previous task
*/
void
DrawWaypointsIconsTitle(Canvas &canvas, const WaypointPtr waypoint_before,
                        const WaypointPtr waypoint_current, unsigned task_size,
                        const NavigatorLook &look_nav) noexcept;
}