// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

enum class TaskPointType : uint8_t;
class OrderedTask;
class ObservationZonePoint;

/**
 * Shape, validation, and distance summary of an ordered task.
 *
 * @param task The Task
 * @param text A buffer written to
 * @param linebreaks True if each summary item should be separated with a line break
 */
void
OrderedTaskSummary(const OrderedTask *task, char *text, bool linebreaks);

void
OrderedTaskPointLabel(TaskPointType type, const char *name,
                      unsigned index, char *buffer);

void
OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, char *radius);
