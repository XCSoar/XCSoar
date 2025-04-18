// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOAAStore.hpp"
#include "util/tstring.hpp"

namespace NOAAFormatter {

void
Format(const NOAAStore::Item &station, tstring &output);

} // namespace NOAAFormatter
