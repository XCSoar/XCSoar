// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "PageSettings.hpp"

#include <span>

class RaspStore;

/**
 * The overlay's own label, without the separator used when it is
 * joined onto a page title. False when the page has no overlay.
 */
bool
FormatOverlayLabel(std::span<char> buffer,
                   const PageLayout &layout,
                   const RaspStore *rasp) noexcept;
