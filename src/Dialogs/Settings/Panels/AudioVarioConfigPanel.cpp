// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AudioVarioConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Audio/Features.hpp"
#include "Audio/VarioGlue.hpp"
#include "Audio/VarioSettings.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

static constexpr StaticEnumChoice switching_modes[] = {
  { VarioSoundSwitchingMode::MANUAL, NC_("Setting", "Manual") },
  { VarioSoundSwitchingMode::AUTO, NC_("Setting", "Auto") },
  nullptr
};

/** the range of a tone frequency, in Hz */
static constexpr int FREQUENCY_MIN = 50, FREQUENCY_MAX = 3000,
  FREQUENCY_STEP = 50;

/**
 * The sound of the vario: whether it plays, how loud, and which tones
 * it plays for which climb rate.
 */
class AudioVarioConfigPanel final : public ConfigListPanel {
  bool enabled;

  /** the volume as a percentage */
  int volume;

  VarioSoundSwitchingMode switching_mode;
  bool dead_band_enabled;

  /** the tones, in Hz */
  int min_frequency, zero_frequency, max_frequency;

  /** the dead band, in m/s */
  double min_dead, max_dead;

private:
  void AddFrequencyItem(const char *caption, const char *help,
                        int &value) noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
AudioVarioConfigPanel::LoadSettings() noexcept
{
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
AudioVarioConfigPanel::AddFrequencyItem(const char *caption,
                                        const char *help,
                                        int &value) noexcept
{
  StaticString<16> hz;
  hz.Format("%d Hz", value);

  AddItem(caption, [this, caption, help, &value](){
    if (PickNumber(caption, help, FREQUENCY_MIN, FREQUENCY_MAX,
                   FREQUENCY_STEP, value,
                   [](StaticString<32> &s, int v){ s.Format("%d Hz", v); }))
      Refresh();
  }, {.value = hz.c_str(), .chevron = true});
}

void
AudioVarioConfigPanel::Fill() noexcept
{
  if (!AudioVarioGlue::HaveAudioVario())
    return;

  AddGroup();

  AddToggleItem(_("Audio Vario"),
                _("Emulate the sound of an electronic vario."),
                enabled);

  AddPercentItem(_("Volume"), _("The audio vario sound volume."),
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

  AddGroup();

  AddVerticalSpeedItem(_("Deadband min. lift"),
                       _("Below this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
                       -5, 0, min_dead, true);

  AddVerticalSpeedItem(_("Deadband max. lift"),
                       _("Above this lift threshold the vario will start to play sounds if the 'Deadband' feature is enabled."),
                       0, 2, max_dead, true);
}

bool
AudioVarioConfigPanel::Save(bool &changed) noexcept
{
  if (!AudioVarioGlue::HaveAudioVario())
    return true;

  auto &settings = CommonInterface::SetUISettings().sound.vario;

  changed |= Profile::Update(ProfileKeys::SoundAudioVario,
                             settings.enabled, enabled);

  if (settings.volume != volume) {
    settings.volume = volume;
    Profile::Set(ProfileKeys::SoundVolume, settings.volume);
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::VarioSoundSwitchingMode,
                             settings.switching_mode, switching_mode);

  changed |= Profile::Update(ProfileKeys::VarioDeadBandEnabled,
                             settings.dead_band_enabled, dead_band_enabled);

  changed |= Profile::Update(ProfileKeys::VarioMinFrequency,
                             settings.min_frequency, (unsigned)min_frequency);

  changed |= Profile::Update(ProfileKeys::VarioZeroFrequency,
                             settings.zero_frequency,
                             (unsigned)zero_frequency);

  changed |= Profile::Update(ProfileKeys::VarioMaxFrequency,
                             settings.max_frequency, (unsigned)max_frequency);

  changed |= Profile::Update(ProfileKeys::VarioDeadBandMin,
                             settings.min_dead, min_dead);

  changed |= Profile::Update(ProfileKeys::VarioDeadBandMax,
                             settings.max_dead, max_dead);

  return true;
}

std::unique_ptr<Widget>
CreateAudioVarioConfigPanel()
{
  return std::make_unique<AudioVarioConfigPanel>();
}
