// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;
struct StaticEnumChoice;

std::unique_ptr<Widget>
CreateLayoutConfigPanel();

extern const StaticEnumChoice info_box_geometry_list[];
