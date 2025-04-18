// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class Path;
class OrderedTask;

/**
 * Throws std::runtime_error on error.
 */
void
SaveTask(Path path, const OrderedTask &task);
