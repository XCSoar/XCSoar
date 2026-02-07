// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Dialogs/dlgAnalysis.hpp"

/**
 * Open the Analysis dialog on the given page after validating
 * that the required backend and data components are available.
 *
 * @return true if the dialog was shown, false if a required
 * component was unavailable
 */
bool
ShowAnalysis(AnalysisPage page) noexcept;
