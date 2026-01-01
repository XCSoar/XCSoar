// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

[[nodiscard]]
AllocatedPath
FindDefaultFont() noexcept;

[[nodiscard]]
AllocatedPath
FindDefaultBoldFont() noexcept;

[[nodiscard]]
AllocatedPath
FindDefaultItalicFont() noexcept;

[[nodiscard]]
AllocatedPath
FindDefaultBoldItalicFont() noexcept;

[[nodiscard]]
AllocatedPath
FindDefaultMonospaceFont() noexcept;
