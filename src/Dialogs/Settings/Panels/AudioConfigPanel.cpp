// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AudioConfigPanel.hpp"
#include "Audio/Features.hpp"

#ifdef HAVE_PCM_PLAYER

#include "Audio/VarioGlue.hpp"
#include "Audio/VarioSettings.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_VOLUME_CONTROLLER
#include "Audio/VolumeController.hpp"
#endif

static constexpr StaticEnumChoice switching_modes[] = {
  { VarioSoundSwitchingMode::MANUAL, NC_("Setting", "Manual") },
  { VarioSoundSwitchingMode::AUTO, NC_("Setting", "Auto") },
  nullptr
};

/** The volume of everything XCSoar plays, and the audio vario. */
class AudioConfigPanel final : public ConfigListPanel {
#ifdef HAVE_VOLUME_CONTROLLER
  int master_volume;
#endif

  bool enabled = false;
  int volume = 0;
  VarioSoundSwitchingMode switching_mode = VarioSoundSwitchingMode::MANUAL;
  bool dead_band_enabled = false;
  unsigned min_frequency = 0, zero_frequency = 0, max_frequency = 0;
  double min_dead = 0, max_dead = 0;

  void AddFrequencyItem(const char *caption, const char *help,
                        unsigned &value) noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
AudioConfigPanel::AddFrequencyItem(const char *caption, const char *help,
                                   unsigned &value) noexcept
{
  StaticString<16> text;
  text.Format("%u Hz", value);

  AddItem(caption, [this, caption, help, &value](){
    int hz = value;
    if (PickNumber(caption, help, 50, 3000, 50, hz,
                   [](StaticString<32> &s, int v){ s.Format("%u Hz", v); })) {
      value = hz;
      Refresh();
    }
  }, {.value = text.c_str(), .chevron = true});
}

void
AudioConfigPanel::LoadSettings() noexcept
{
#ifdef HAVE_VOLUME_CONTROLLER
  master_volume = CommonInterface::GetUISettings().sound.master_volume;
#endif

  if (!AudioVarioGlue::HaveAudioVario())
    return;

  const auto &settings = CommonInterface::GetUISettings().sound.vario;
  enabled = settings.enabled;
  volume = settings.volume;
  switching_mode = settings.switching_mode;
  dead_band_enabled = settings.dead_band_enabled;
  min_frequency = settings.min_frequency;
  zero_frequency = settings.zero_frequency;
  max_frequency = settings.max_frequency;
  min_dead = settings.min_dead;
  max_dead = settings.max_dead;
}

void
AudioConfigPanel::Fill() noexcept
{
#ifdef HAVE_VOLUME_CONTROLLER
  AddGroup();

  AddPercentItem(_("Master Volume"),
                 _("The overall audio output volume."),
                 0, VolumeController::GetMaxValue(), 1, master_volume);
#endif

  if (!AudioVarioGlue::HaveAudioVario())
    return;

  AddGroup(_("Audio Vario"));

  AddToggleItem(_("Audio Vario"),
                _("Emulate the sound of an electronic vario."),
                enabled);

  if (!enabled)
    return;

  AddPercentItem(_("Vario Volume"),
                 _("The audio vario sound volume."),
                 0, 100, 1, volume);

  AddEnumItem(C_("Setting", "Mode switching"),
              _("Choose whether the audio vario stays in manual mode or switches automatically between Vario in circling and STF in cruise. Manual mode starts in Vario after each restart and can be changed by external input events. In the built-in simulator, STF audio needs valid airspeed and total-energy vario input; without those, manual STF is silent and auto cruise falls back to vario."),
              switching_modes, switching_mode);

  AddToggleItem(_("Enable Deadband"),
                _("Mute the audio output in when the current lift is in a "
                  "certain range around zero"),
                dead_band_enabled);

  if (!IsExpert())
    return;

  AddGroup();

  AddFrequencyItem(_("Min. Frequency"),
                   _("The tone frequency that is played at maximum sink rate."),
                   min_frequency);
  AddFrequencyItem(_("Zero Frequency"),
                   _("The tone frequency that is played at zero climb rate."),
                   zero_frequency);
  AddFrequencyItem(_("Max. Frequency"),
                   _("The tone frequency that is played at maximum climb rate."),
                   max_frequency);

  if (!dead_band_enabled)
    return;

  AddVerticalSpeedItem(_("Deadband min. lift"),
                       _("Below this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
                       -5, 0, min_dead, true);
  AddVerticalSpeedItem(_("Deadband max. lift"),
                       _("Above this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
                       0, 2, max_dead, false);
}

bool
AudioConfigPanel::Save(bool &changed) noexcept
{
#ifdef HAVE_VOLUME_CONTROLLER
  auto &sound = CommonInterface::SetUISettings().sound;

  changed |= Profile::Update(ProfileKeys::MasterAudioVolume,
                             sound.master_volume, uint8_t(master_volume));
#endif

  if (!AudioVarioGlue::HaveAudioVario())
    return true;

  auto &settings = CommonInterface::SetUISettings().sound.vario;

  changed |= Profile::Update(ProfileKeys::SoundAudioVario,
                             settings.enabled, enabled);
  changed |= Profile::Update(ProfileKeys::SoundVolume,
                             settings.volume, uint8_t(volume));
  changed |= Profile::Update(ProfileKeys::VarioSoundSwitchingMode,
                             settings.switching_mode, switching_mode);
  changed |= Profile::Update(ProfileKeys::VarioDeadBandEnabled,
                             settings.dead_band_enabled, dead_band_enabled);
  changed |= Profile::Update(ProfileKeys::VarioMinFrequency,
                             settings.min_frequency, min_frequency);
  changed |= Profile::Update(ProfileKeys::VarioZeroFrequency,
                             settings.zero_frequency, zero_frequency);
  changed |= Profile::Update(ProfileKeys::VarioMaxFrequency,
                             settings.max_frequency, max_frequency);
  changed |= Profile::Update(ProfileKeys::VarioDeadBandMin,
                             settings.min_dead, min_dead);
  changed |= Profile::Update(ProfileKeys::VarioDeadBandMax,
                             settings.max_dead, max_dead);

  return true;
}

std::unique_ptr<Widget>
CreateAudioConfigPanel()
{
  return std::make_unique<AudioConfigPanel>();
}

#endif /* HAVE_PCM_PLAYER */
