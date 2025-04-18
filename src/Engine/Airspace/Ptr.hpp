// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class AbstractAirspace;

using AirspacePtr = std::shared_ptr<AbstractAirspace>;
using ConstAirspacePtr = std::shared_ptr<const AbstractAirspace>;
