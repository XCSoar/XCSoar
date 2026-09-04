// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Factory.hpp"
#include "Service.hpp"
#include "StateFile.hpp"

std::unique_ptr<UpdateService>
CreateUpdateService([[maybe_unused]] Repository::Service *repository)
{
  return std::make_unique<UpdateService>(LoadUpdateState(), SaveUpdateState);
}
