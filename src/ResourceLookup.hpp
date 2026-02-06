// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ResourceId.hpp"

/**
 * Look up a ResourceId by its symbolic name (e.g. "IDB_TITLE_HD").
 *
 * This is used by the markdown image renderer to resolve
 * "resource:IDB_NAME" URLs to loadable bitmap resources.
 *
 * @return The matching ResourceId, or ResourceId::Null() if not found
 */
[[gnu::pure]]
ResourceId
LookupResourceByName(const char *name) noexcept;
