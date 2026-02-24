// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class RaspStore;

std::shared_ptr<RaspStore>
LoadConfiguredRasp(bool legacy_default = true) noexcept;

