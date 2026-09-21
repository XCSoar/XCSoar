// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AnalysisPages.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Dialogs/Dialogs.h"
#include "Computer/GlideComputer.hpp"

void
OpenAnalysisPageDetails(AnalysisPage page,
                        GlideComputer &glide_computer) noexcept
{
  switch (page) {
  case AnalysisPage::BAROGRAPH:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::CLIMB:
  case AnalysisPage::TASK:
  case AnalysisPage::TASK_SPEED:
    dlgStatusShowModal(2);
    break;

  case AnalysisPage::WIND:
    ShowWindSettingsDialog();
    break;

  case AnalysisPage::POLAR:
  case AnalysisPage::TEMPTRACE:
  case AnalysisPage::MACCREADY:
    dlgBasicSettingsShowModal();
    break;

  case AnalysisPage::AIRSPACE:
    dlgAirspaceWarningsShowModal(glide_computer.GetAirspaceWarnings());
    break;

  case AnalysisPage::THERMAL_BAND:
  case AnalysisPage::VARIO_HISTOGRAM:
  case AnalysisPage::CONTEST:
  case AnalysisPage::COUNT:
    break;
  }
}
