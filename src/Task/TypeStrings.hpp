// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstdint>

enum class TaskPointType : uint8_t;
enum class TaskFactoryType : uint8_t;
enum class TaskPointFactoryType : uint8_t;

[[gnu::const]]
const char *
OrderedTaskFactoryDescription(TaskFactoryType type);

[[gnu::const]]
const char *
OrderedTaskFactoryName(TaskFactoryType type);

[[gnu::const]]
const char *
OrderedTaskPointDescription(TaskPointFactoryType type);

[[gnu::const]]
const char *
OrderedTaskPointName(TaskPointFactoryType type);
