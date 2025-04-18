// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AudioVarioConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Float.hpp"
#include "UIGlobals.hpp"
#include "Audio/Features.hpp"
#include "Audio/VarioGlue.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"

enum ControlIndex {
  Enabled,
  Volume,
  DEAD_BAND_ENABLED,
  SPACER,
  MIN_FREQUENCY,
  ZERO_FREQUENCY,
  MAX_FREQUENCY,
  SPACER2,
  DEAD_BAND_MIN,
  DEAD_BAND_MAX,
};


class AudioVarioConfigPanel final : public RowFormWidget {
public:
  AudioVarioConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
AudioVarioConfigPanel::Prepare(ContainerWindow &parent,
                               const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  if (!AudioVarioGlue::HaveAudioVario())
    return;

  const auto &settings = CommonInterface::GetUISettings().sound.vario;

  AddBoolean(_("Audio vario"),
             _("Emulate the sound of an electronic vario."),
             settings.enabled);

  AddInteger(_("Volume"), nullptr, _T("%u %%"), _T("%u"),
             0, 100, 1, settings.volume);

  AddBoolean(_("Enable Deadband"),
             _("Mute the audio output in when the current lift is in a certain "
               "range around zero"), settings.dead_band_enabled);

  AddSpacer();
  SetExpertRow(SPACER);

  AddInteger(_("Min. Frequency"),
             _("The tone frequency that is played at maximum sink rate."),
             _T("%u Hz"), _T("%u"),
             50, 3000, 50, settings.min_frequency);
  SetExpertRow(MIN_FREQUENCY);

  AddInteger(_("Zero Frequency"),
             _("The tone frequency that is played at zero climb rate."),
             _T("%u Hz"), _T("%u"),
             50, 3000, 50, settings.zero_frequency);
  SetExpertRow(ZERO_FREQUENCY);

  AddInteger(_("Max. Frequency"),
             _("The tone frequency that is played at maximum climb rate."),
             _T("%u Hz"), _T("%u"),
             50, 3000, 50, settings.max_frequency);
  SetExpertRow(MAX_FREQUENCY);

  AddSpacer();
  SetExpertRow(SPACER2);

  AddFloat(_("Deadband min. lift"),
           _("Below this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
           _T("%.1f %s"), _T("%.1f"),
           Units::ToUserVSpeed(-5), 0,
           GetUserVerticalSpeedStep(), false, UnitGroup::VERTICAL_SPEED,
           settings.min_dead);
  SetExpertRow(DEAD_BAND_MIN);
  DataFieldFloat &db_min = (DataFieldFloat &)GetDataField(DEAD_BAND_MIN);
  db_min.SetFormat(GetUserVerticalSpeedFormat(false, true));

  AddFloat(_("Deadband max. lift"),
           _("Above this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
           _T("%.1f %s"), _T("%.1f"),
           0, Units::ToUserVSpeed(2),
           GetUserVerticalSpeedStep(), false, UnitGroup::VERTICAL_SPEED,
           settings.max_dead);
  SetExpertRow(DEAD_BAND_MAX);
  DataFieldFloat &db_max = (DataFieldFloat &)GetDataField(DEAD_BAND_MAX);
  db_max.SetFormat(GetUserVerticalSpeedFormat(false, true));
}

bool
AudioVarioConfigPanel::Save(bool &changed) noexcept
{
  if (!AudioVarioGlue::HaveAudioVario())
    return true;

  auto &settings = CommonInterface::SetUISettings().sound.vario;

  changed |= SaveValue(Enabled, ProfileKeys::SoundAudioVario,
                       settings.enabled);

  changed |= SaveValueInteger(Volume, ProfileKeys::SoundVolume,
                              settings.volume);

  changed |= SaveValue(DEAD_BAND_ENABLED, ProfileKeys::VarioDeadBandEnabled,
                       settings.dead_band_enabled);

  changed |= SaveValueInteger(MIN_FREQUENCY, ProfileKeys::VarioMinFrequency,
                              settings.min_frequency);

  changed |= SaveValueInteger(ZERO_FREQUENCY, ProfileKeys::VarioZeroFrequency,
                              settings.zero_frequency);

  changed |= SaveValueInteger(MAX_FREQUENCY, ProfileKeys::VarioMaxFrequency,
                              settings.max_frequency);

  changed |= SaveValue(DEAD_BAND_MIN, UnitGroup::VERTICAL_SPEED,
                       ProfileKeys::VarioDeadBandMin, settings.min_dead);

  changed |= SaveValue(DEAD_BAND_MAX, UnitGroup::VERTICAL_SPEED,
                       ProfileKeys::VarioDeadBandMax, settings.max_dead);

  return true;
}

std::unique_ptr<Widget>
CreateAudioVarioConfigPanel()
{
  return std::make_unique<AudioVarioConfigPanel>();
}
