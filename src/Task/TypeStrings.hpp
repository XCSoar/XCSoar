// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>
#include <cstdint>

enum class TaskPointType : uint8_t;
enum class TaskFactoryType : uint8_t;
enum class TaskPointFactoryType : uint8_t;

[[gnu::const]]
const TCHAR *
OrderedTaskFactoryDescription(TaskFactoryType type);

[[gnu::const]]
const TCHAR *
OrderedTaskFactoryName(TaskFactoryType type);

[[gnu::const]]
const TCHAR *
OrderedTaskPointDescription(TaskPointFactoryType type);

[[gnu::const]]
const TCHAR *
OrderedTaskPointName(TaskPointFactoryType type);
