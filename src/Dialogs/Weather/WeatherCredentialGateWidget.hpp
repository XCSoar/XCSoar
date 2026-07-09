// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/Widget.hpp"

#include <functional>
#include <memory>

/**
 * Two-phase weather tab: inline credential config, then main UI.
 */
std::unique_ptr<Widget>
CreateWeatherCredentialGateWidget(
  std::function<bool()> credentials_ok,
  std::unique_ptr<Widget> (*create_config_panel)(),
  std::function<std::unique_ptr<Widget>()> create_main) noexcept;
