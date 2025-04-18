// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AudioConfigPanel.hpp"
#include "Audio/Features.hpp"

#ifdef HAVE_VOLUME_CONTROLLER

#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "Audio/VolumeController.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Widget/RowFormWidget.hpp"

enum ControlIndex {
  MasterVolume,
};


class AudioConfigPanel final : public RowFormWidget {
public:
  AudioConfigPanel() : RowFormWidget(UIGlobals::GetDialogLook()) {
  }

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
AudioConfigPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  const auto &settings = CommonInterface::GetUISettings().sound;

  AddInteger(_("Master Volume"), nullptr, _T("%d %%"), _T("%d"),
             0, VolumeController::GetMaxValue(), 1, settings.master_volume);
}

bool
AudioConfigPanel::Save(bool &changed) noexcept
{
  auto &settings = CommonInterface::SetUISettings().sound;

  changed |= SaveValueInteger(MasterVolume, ProfileKeys::MasterAudioVolume,
                              settings.master_volume);

  return true;
}

std::unique_ptr<Widget>
CreateAudioConfigPanel()
{
  return std::make_unique<AudioConfigPanel>();
}

#endif /* HAVE_VOLUME_CONTROLLER */
