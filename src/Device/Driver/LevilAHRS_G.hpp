// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class NMEAInputLine;
struct NMEAInfo;

extern const struct DeviceRegister levil_driver;

bool ParseRPYL(NMEAInputLine &line, NMEAInfo &info);
bool ParseAPENV1(NMEAInputLine &line, NMEAInfo &info);
