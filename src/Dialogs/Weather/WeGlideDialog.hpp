// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class Widget;

/** Info → Weather tab: sign-in gate, then layer and apply. */
std::unique_ptr<Widget>
CreateWeGlideWeatherTab() noexcept;
