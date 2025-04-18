// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ChartRenderer;
struct NMEAInfo;
struct DerivedInfo;
class TaskManager;

// draw task leg lines and, if y>=0, labels
void
RenderTaskLegs(ChartRenderer &chart,
               const TaskManager &task_manager,
               const NMEAInfo& basic,
               const DerivedInfo& calculated,
               double y=0.5);
