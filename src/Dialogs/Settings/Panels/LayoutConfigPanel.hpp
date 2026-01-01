// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>
#include "Form/DataField/Enum.hpp"

class Widget;

std::unique_ptr<Widget>
CreateLayoutConfigPanel();

extern const StaticEnumChoice info_box_geometry_list[];
