// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ShowAnalysis.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Computer/GlideComputer.hpp"

bool
ShowAnalysis(AnalysisPage page) noexcept
{
  if (!backend_components || !backend_components->glide_computer ||
      !data_components || !data_components->airspaces ||
      !data_components->terrain)
    return false;

  dlgAnalysisShowModal(UIGlobals::GetMainWindow(),
                       UIGlobals::GetLook(),
                       CommonInterface::Full(),
                       *backend_components->glide_computer,
                       data_components->airspaces.get(),
                       data_components->terrain.get(),
                       page);
  return true;
}
