// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

class ProtectedAirspaceWarningManager;

void
dlgAirspaceWarningsShowModal(ProtectedAirspaceWarningManager &_warnings,
                             bool auto_close=false);

bool
dlgAirspaceWarningVisible();
