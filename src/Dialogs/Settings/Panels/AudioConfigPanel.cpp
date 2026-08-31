// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AudioConfigPanel.hpp"
#include "Audio/Features.hpp"

#ifdef HAVE_PCM_PLAYER

#include "Audio/VarioGlue.hpp"
#include "Audio/VarioSettings.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Float.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "Widget/RowFormWidget.hpp"

#ifdef HAVE_VOLUME_CONTROLLER
#include "Audio/VolumeController.hpp"
#endif

enum ControlIndex {
#ifdef HAVE_VOLUME_CONTROLLER
  MASTER_VOLUME,
  MASTER_VOLUME_SPACER,
#endif
  ENABLED,
  VOLUME,
  SWITCHING_MODE,
  DEAD_BAND_ENABLED,
  SPACER,
  MIN_FREQUENCY,
  ZERO_FREQUENCY,
  MAX_FREQUENCY,
  SPACER2,
  DEAD_BAND_MIN,
  DEAD_BAND_MAX,
};

static constexpr StaticEnumChoice switching_modes[] = {
  { VarioSoundSwitchingMode::MANUAL, NC_("Setting", "Manual") },
  { VarioSoundSwitchingMode::AUTO, NC_("Setting", "Auto") },
  nullptr
};

class AudioConfigPanel final : public RowFormWidget {
public:
  AudioConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
AudioConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_VOLUME_CONTROLLER
  const auto &sound = CommonInterface::GetUISettings().sound;

  AddInteger(_("Master Volume"),
             _("The overall audio output volume."),
             "%d %%", "%d",
             0, VolumeController::GetMaxValue(), 1, sound.master_volume);
#endif

  if (!AudioVarioGlue::HaveAudioVario())
    return;

#ifdef HAVE_VOLUME_CONTROLLER
  AddSpacer();
#endif

  const auto &settings = CommonInterface::GetUISettings().sound.vario;

  AddBoolean(_("Audio Vario"),
             _("Emulate the sound of an electronic vario."),
             settings.enabled);

  AddInteger(_("Vario Volume"),
             _("The audio vario sound volume."), "%u %%", "%u",
             0, 100, 1, settings.volume);

  AddEnum(C_("Setting", "Mode switching"),
      _("Choose whether the audio vario stays in manual mode or switches automatically between Vario in circling and STF in cruise. Manual mode starts in Vario after each restart and can be changed by external input events. In the built-in simulator, STF audio needs valid airspeed and total-energy vario input; without those, manual STF is silent and auto cruise falls back to vario."),
          switching_modes, (unsigned)settings.switching_mode);

  AddBoolean(_("Enable Deadband"),
             _("Mute the audio output in when the current lift is in a "
               "certain range around zero"), settings.dead_band_enabled);

  AddSpacer();
  SetExpertRow(SPACER);

  AddInteger(_("Min. Frequency"),
             _("The tone frequency that is played at maximum sink rate."),
             "%u Hz", "%u",
             50, 3000, 50, settings.min_frequency);
  SetExpertRow(MIN_FREQUENCY);

  AddInteger(_("Zero Frequency"),
             _("The tone frequency that is played at zero climb rate."),
             "%u Hz", "%u",
             50, 3000, 50, settings.zero_frequency);
  SetExpertRow(ZERO_FREQUENCY);

  AddInteger(_("Max. Frequency"),
             _("The tone frequency that is played at maximum climb rate."),
             "%u Hz", "%u",
             50, 3000, 50, settings.max_frequency);
  SetExpertRow(MAX_FREQUENCY);

  AddSpacer();
  SetExpertRow(SPACER2);

  AddFloat(_("Deadband min. lift"),
           _("Below this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
           "%.1f %s", "%.1f",
           Units::ToUserVSpeed(-5), 0,
           GetUserVerticalSpeedStep(), false, UnitGroup::VERTICAL_SPEED,
           settings.min_dead);
  SetExpertRow(DEAD_BAND_MIN);
  DataFieldFloat &db_min = (DataFieldFloat &)GetDataField(DEAD_BAND_MIN);
  db_min.SetFormat(GetUserVerticalSpeedFormat(false, true));

  AddFloat(_("Deadband max. lift"),
           _("Above this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
           "%.1f %s", "%.1f",
           0, Units::ToUserVSpeed(2),
           GetUserVerticalSpeedStep(), false, UnitGroup::VERTICAL_SPEED,
           settings.max_dead);
  SetExpertRow(DEAD_BAND_MAX);
  DataFieldFloat &db_max = (DataFieldFloat &)GetDataField(DEAD_BAND_MAX);
  db_max.SetFormat(GetUserVerticalSpeedFormat(false, true));
}

bool
AudioConfigPanel::Save(bool &changed) noexcept
{
#ifdef HAVE_VOLUME_CONTROLLER
  auto &sound = CommonInterface::SetUISettings().sound;

  changed |= SaveValueInteger(MASTER_VOLUME, ProfileKeys::MasterAudioVolume,
                              sound.master_volume);
#endif

  if (!AudioVarioGlue::HaveAudioVario())
    return true;

  auto &settings = CommonInterface::SetUISettings().sound.vario;

  changed |= SaveValue(ENABLED, ProfileKeys::SoundAudioVario,
                       settings.enabled);

  changed |= SaveValueInteger(VOLUME, ProfileKeys::SoundVolume,
                              settings.volume);

  changed |= SaveValueEnum(SWITCHING_MODE, ProfileKeys::VarioSoundSwitchingMode,
                           settings.switching_mode);

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
CreateAudioConfigPanel()
{
  return std::make_unique<AudioConfigPanel>();
}

#endif /* HAVE_PCM_PLAYER */
