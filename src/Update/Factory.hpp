// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class UpdateService;
namespace Repository { class Service; }

/** Compose the update subsystem from the providers available in this build. */
std::unique_ptr<UpdateService>
CreateUpdateService(Repository::Service *repository);
