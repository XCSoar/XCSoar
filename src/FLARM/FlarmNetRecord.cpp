// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FlarmNetRecord.hpp"
#include "Id.hpp"

FlarmId
FlarmNetRecord::GetId() const noexcept
{
  return FlarmId::Parse(this->id, NULL);
};
