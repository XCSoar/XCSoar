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
    freq.Format(data.value.data(), data.value.capacity());
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

static void
UpdateInfoBoxTransponderCode(InfoBoxData &data, TransponderCode code) noexcept
{
  if(code.IsDefined()) {
    code.Format(data.value.data(), data.value.capacity());
  }
  else {
    data.SetValueInvalid();
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

static constexpr InfoBoxPanel transponder_code_panels[] = {
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

void
InfoBoxContentTransponderCode::Update(InfoBoxData &data) noexcept
{
  const auto &settings_transponder =
    CommonInterface::GetComputerSettings().transponder;
  UpdateInfoBoxTransponderCode(data, settings_transponder.transponder_code);
}

const InfoBoxPanel *
InfoBoxContentTransponderCode::GetDialogContent() noexcept
{
  return transponder_code_panels;
}
