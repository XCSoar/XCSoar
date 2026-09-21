// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"

#ifdef HAVE_HTTP

#include "util/StaticString.hxx"

class SkySightClient;

namespace SkySight {
struct Layer;
}

/**
 * Second-row status text for a selected SkySight layer.
 */
[[nodiscard]]
StaticString<256>
FormatSelectedLayerStatus(const SkySightClient &client,
                          const SkySight::Layer &layer) noexcept;

#endif
