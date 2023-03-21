// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
 
#pragma once

#include "Ptr.hpp"

/**
 * Generic visitor for objects in the Airspaces container
 */
class AirspaceVisitor {
public:
  virtual void Visit(ConstAirspacePtr airspace) noexcept = 0;
};
