// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class DeviceDescriptor;
class OperationEnvironment;

void
VarioWriteNMEA(const TCHAR *Text, OperationEnvironment &env);

DeviceDescriptor *devVarioFindVega();

void devStartup();
void devShutdown();
void devRestart();
