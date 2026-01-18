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
#include "util/ConvertString.hpp"

#include <tchar.h>

static void
UpdateInfoBoxFrequency(InfoBoxData &data, const RadioFrequency freq,
                       const TCHAR *freq_name) noexcept
{
  if(freq.IsDefined()) {
    const auto freq_str = freq.Format();
    const UTF8ToWideConverter freq_wide(freq_str.c_str());
    if (freq_wide.IsValid())
      data.SetValue(freq_wide.c_str());
    else
      data.SetValueInvalid();
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
UpdateInfoBoxTransponderCode(InfoBoxData &data, 
                             TransponderCode code,
                             TransponderMode mode) noexcept
{
  if(code.IsDefined()) {
    const auto code_str = code.Format();
    const UTF8ToWideConverter code_wide(code_str.c_str());
    if (code_wide.IsValid()) {
      data.SetValue(code_wide.c_str());
      const auto value = code.GetCode();
      // Emergency squawk codes
      if (value == 7500 || value == 7600 || value == 7700) {
        data.SetValueColor(1);
      } else {
        data.SetValueColor(0);
      }
    } else {
      data.SetValueInvalid();
    }
  }
  else {
    data.SetValueInvalid();
  }

  if(mode.IsDefined()) {
    const char *mode_key = mode.GetModeString();
    const char *mode_text = _utf8(mode_key);
    const UTF8ToWideConverter mode_wide(mode_text);
    if (mode_wide.IsValid())
      data.SetComment(mode_wide.c_str());
    else
      data.SetCommentInvalid();

    if (mode.mode == TransponderMode::IDENT) {
      data.SetCommentColor(5);
    } else if (mode.mode == TransponderMode::ALT) {
      data.SetCommentColor(3);
    } else {
      data.SetCommentColor(0);
    }
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
  UpdateInfoBoxTransponderCode(data, 
                               settings_transponder.transponder_code,
                               settings_transponder.transponder_mode);
}

const InfoBoxPanel *
InfoBoxContentTransponderCode::GetDialogContent() noexcept
{
  return transponder_code_panels;
}
