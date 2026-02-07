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

  if (!basic.engine.cht_temperature_available.IsValid()) {
    data.SetInvalid();
    return;
  }

  data.FmtValue("{:3.0f}", basic.engine.cht_temperature.ToUser());
  data.SetValueUnit(Units::current.temperature_unit);
}

void
UpdateInfoBoxContentEGT(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.engine.egt_temperature_available.IsValid()) {
    data.SetInvalid();
    return;
  }

  data.FmtValue("{:3.0f}", basic.engine.egt_temperature.ToUser());
  data.SetValueUnit(Units::current.temperature_unit);
}

void
UpdateInfoBoxContentRPM(InfoBoxData &data) noexcept
{
  const NMEAInfo &basic = CommonInterface::Basic();

  if (!basic.engine.revolutions_per_second_available.IsValid()) {
    data.SetInvalid();
    return;
  }  

  data.FmtValue("{}", Units::ToUserRotation(basic.engine.revolutions_per_second));
  data.SetValueUnit(Units::current.rotation_unit);
}
