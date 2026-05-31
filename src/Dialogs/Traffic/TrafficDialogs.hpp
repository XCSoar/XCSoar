// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class FlarmId;

void
dlgTeamCodeShowModal();

[[nodiscard]] bool
dlgFlarmTrafficDetailsShowModal(FlarmId id) noexcept;

void
TrafficListDialog();

FlarmId
PickFlarmTraffic(const char *title, FlarmId array[], unsigned count);
