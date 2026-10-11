// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

bool PlayResource(const char *resource_name);

/**
 * Play the sound resources one after another, replacing what an
 * earlier call has not played yet.  This speaks an alert made of
 * several clips.
 */
bool PlayResources(std::span<const char *const> resource_names);
