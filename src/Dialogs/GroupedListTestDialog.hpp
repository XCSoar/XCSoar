// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Show a #GroupedListWidget filled with example groups.  A scratch
 * dialog for looking at the widget inside the running program.
 */
void
ShowGroupedListTestDialog() noexcept;

/**
 * Show the configuration menu of XCSoar, built from
 * #GroupedListWidget pages: one page opens the next.  A scratch
 * dialog which tries the widget on a menu which exists.
 */
void
ShowGroupedListMenuDialog() noexcept;
