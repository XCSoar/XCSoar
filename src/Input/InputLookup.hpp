// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/tstring_view.hxx"

#include <tchar.h>

typedef void (*pt2Event)(const TCHAR *);

namespace InputEvents {

[[gnu::pure]]
int findGCE(const TCHAR *data);

[[gnu::pure]]
int findNE(const TCHAR *data);

[[gnu::pure]]
pt2Event
findEvent(tstring_view name) noexcept;

} // namespace InputEvents
