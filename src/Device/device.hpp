// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

struct SystemSettings;
class MultipleDevices;

void
devStartup(MultipleDevices &devices, const SystemSettings &settings);

void
devRestart(MultipleDevices &devices, const SystemSettings &settings);
