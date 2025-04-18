// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstdint>

enum class TaskPointType : uint8_t;
class OrderedTask;
class ObservationZonePoint;

/**
 *
 * @param task The Task
 * @param text A buffer written to
 * @param linebreaks True if each summary item should be separated with a line break
 */
void
OrderedTaskSummary(const OrderedTask *task, TCHAR *text, bool linebreaks);

void
OrderedTaskPointLabel(TaskPointType type, const TCHAR *name,
                      unsigned index, TCHAR *buffer);

void OrderedTaskPointRadiusLabel(const ObservationZonePoint &ozp, TCHAR* radius);

bool
OrderedTaskSave(OrderedTask &task);
