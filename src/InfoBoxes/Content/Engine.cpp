// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Content/Engine.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Language/Language.hpp"

#include <tchar.h>

void
UpdateInfoBoxContentCHT(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.engine_state.cht_temperature_available.IsValid()) {
    data.SetInvalid();
    return;
  }

  data.SetValue(_T("%3.0f"), basic.engine_state.cht_temperature.ToUser());
  data.SetValueUnit(Units::current.temperature_unit);
}

void
UpdateInfoBoxContentEGT(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.engine_state.egt_temperature_available.IsValid()) {
    data.SetInvalid();
    return;
  }
  data.SetValue(_T("%3.0f"), basic.engine_state.egt_temperature.ToUser());
  data.SetValueUnit(Units::current.temperature_unit);
}

void
UpdateInfoBoxContentRPM(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();
  if (!basic.engine_state.revs_per_sec_available.IsValid()) {
    data.SetInvalid();
    return;
  }  
  data.SetValue(_T("%.0f"), basic.engine_state.revs_per_sec * 60);
  data.SetComment(_("rpm"));
}
