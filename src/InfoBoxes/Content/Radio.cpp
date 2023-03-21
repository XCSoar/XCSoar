// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxes/Panel/Panel.hpp"
#include "InfoBoxes/Content/Radio.hpp"
#include "InfoBoxes/Panel/RadioEdit.hpp"
#include "InfoBoxes/Data.hpp"
#include "Interface.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Language/Language.hpp"

#include <tchar.h>

static void
UpdateInfoBoxFrequency(InfoBoxData &data, const RadioFrequency freq,
                       const TCHAR *freq_name) noexcept
{
  if(freq.IsDefined()) {
    data.FormatValue(_T("%u.%03u"), freq.GetKiloHertz() / 1000, freq.GetKiloHertz() % 1000);
  }
  else {
    data.SetValueInvalid();
  }
  if(freq.IsDefined() && freq_name != nullptr) {
    data.SetComment(freq_name);
  }
  else {
    data.SetCommentInvalid();
  }
}

static constexpr InfoBoxPanel active_frequency_panels[] = {
  { N_("Edit"), LoadActiveRadioFrequencyEditPanel },
  { nullptr, nullptr }
};

static constexpr InfoBoxPanel standby_frequency_panels[] = {
  { N_("Edit"), LoadStandbyRadioFrequencyEditPanel },
  { nullptr, nullptr }
};

const InfoBoxPanel *
InfoBoxContentActiveRadioFrequency::GetDialogContent() noexcept
{
  return active_frequency_panels;
}

void
InfoBoxContentActiveRadioFrequency::Update(InfoBoxData &data) noexcept
{
  const auto &settings_radio =
    CommonInterface::GetComputerSettings().radio;
    data.SetValueColor(3);
  UpdateInfoBoxFrequency(data, settings_radio.active_frequency, settings_radio.active_name);
}

const InfoBoxPanel *
InfoBoxContentStandbyRadioFrequency::GetDialogContent() noexcept
{
  return standby_frequency_panels;
}

void
InfoBoxContentStandbyRadioFrequency::Update(InfoBoxData &data) noexcept
{
  const auto &settings_radio =
    CommonInterface::GetComputerSettings().radio;
    data.SetValueColor(2);
  UpdateInfoBoxFrequency(data, settings_radio.standby_frequency, settings_radio.standby_name);
}
