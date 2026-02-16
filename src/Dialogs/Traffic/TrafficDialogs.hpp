// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class FlarmId;

void
dlgTeamCodeShowModal();

void
dlgFlarmTrafficDetailsShowModal(FlarmId id);

void
TrafficListDialog();

FlarmId
PickFlarmTraffic(const char *title, FlarmId array[], unsigned count);
