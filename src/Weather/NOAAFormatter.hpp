// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NOAAStore.hpp"

#include <string>

namespace NOAAFormatter {

void
Format(const NOAAStore::Item &station, std::string &output);

} // namespace NOAAFormatter
