// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmNetRecord.hpp"
#include "FlarmId.hpp"

FlarmId
FlarmNetRecord::GetId() const
{
  return FlarmId::Parse(this->id, NULL);
};
