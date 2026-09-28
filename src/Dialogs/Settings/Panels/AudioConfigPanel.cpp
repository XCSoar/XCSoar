// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AudioConfigPanel.hpp"
#include "Audio/Features.hpp"

#ifdef HAVE_VOLUME_CONTROLLER

#include "ConfigListPanel.hpp"
#include "Interface.hpp"
#include "Audio/VolumeController.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

/** The volume of everything XCSoar plays. */
class AudioConfigPanel final : public ConfigListPanel {
  int master_volume;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
AudioConfigPanel::LoadSettings() noexcept
{
  master_volume = CommonInterface::GetUISettings().sound.master_volume;
}

void
AudioConfigPanel::Fill() noexcept
{
  AddGroup();

  AddPercentItem(_("Master Volume"),
                 _("The overall audio output volume."),
                 0, VolumeController::GetMaxValue(), 5, master_volume);
}

bool
AudioConfigPanel::Save(bool &changed) noexcept
{
  auto &settings = CommonInterface::SetUISettings().sound;

  changed |= Profile::Update(ProfileKeys::MasterAudioVolume,
                             settings.master_volume,
                             uint8_t(master_volume));

  return true;
}

std::unique_ptr<Widget>
CreateAudioConfigPanel()
{
  return std::make_unique<AudioConfigPanel>();
}

#endif /* HAVE_VOLUME_CONTROLLER */
