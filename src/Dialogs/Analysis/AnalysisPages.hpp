// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Dialogs/dlgAnalysis.hpp"

class GlideComputer;

/**
 * Open the settings or status dialog for the current analysis page.
 */
void
OpenAnalysisPageDetails(AnalysisPage page,
                        GlideComputer &glide_computer) noexcept;
