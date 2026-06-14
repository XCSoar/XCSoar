// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Repository/FileType.hpp"
#include "UtilsSettings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  RaspFile,
};

class RaspConfigPanel final : public RowFormWidget {
public:
  RaspConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RaspConfigPanel::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  AddFile("RASP",
          _("Regional Atmospheric Soaring Prediction file providing "
            "weather forecasts for soaring. Displays color-coded map "
            "overlays for thermal strength, boundary layer winds, "
            "cloud cover, and other soaring-relevant parameters at "
            "various forecast times throughout the day."),
          ProfileKeys::RaspFile,
          GetFileTypePatterns(FileType::RASP),
          FileType::RASP);
}

bool
RaspConfigPanel::Save(bool &_changed) noexcept
{
  RaspFileChanged = SaveValueFileReader(RaspFile, ProfileKeys::RaspFile);

  _changed |= RaspFileChanged;

  return true;
}

std::unique_ptr<Widget>
CreateRaspConfigPanel()
{
  return std::make_unique<RaspConfigPanel>();
}
