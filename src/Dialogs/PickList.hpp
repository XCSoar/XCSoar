// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"

/**
 * Ask for a number from the stepped values the old list offered.
 * The current value is under the cursor.  Choosing a row sets it.
 * A range which does not fit one page keeps "More" at the ends.
 *
 * @return true when a value was chosen
 */
bool
PickList(const char *caption, double &value,
         double min_value, double max_value, double step,
         void (*format)(double value, StaticString<64> &text));
