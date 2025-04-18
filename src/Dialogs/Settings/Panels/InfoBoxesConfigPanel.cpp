// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxesConfigPanel.hpp"
#include "../dlgConfigInfoboxes.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Current.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Form/Button.hpp"
#include "Interface.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

class InfoBoxesConfigPanel final
  : public RowFormWidget {
public:
  InfoBoxesConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

  void OnAction(int id) noexcept;
};

void
InfoBoxesConfigPanel::OnAction(int id) noexcept
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;

  unsigned i = (unsigned)id;
  InfoBoxSettings::Panel &data = settings.panels[i];

  bool changed =
    dlgConfigInfoboxesShowModal(UIGlobals::GetMainWindow(),
                                UIGlobals::GetDialogLook(),
                                UIGlobals::GetLook().info_box,
                                InfoBoxManager::layout.geometry, data,
                                i >= InfoBoxSettings::PREASSIGNED_PANELS);
  if (changed) {
    Profile::Save(Profile::map, data, i);
    Profile::Save();
    ((Button &)GetRow(i)).SetCaption(gettext(data.name));
  }
}

void
InfoBoxesConfigPanel::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const InfoBoxSettings &settings = CommonInterface::GetUISettings().info_boxes;

  RowFormWidget::Prepare(parent, rc);

  for (unsigned i = 0; i < InfoBoxSettings::MAX_PANELS; i++) {
    const InfoBoxSettings::Panel &data = settings.panels[i];

    AddButton(gettext(data.name), [this, i](){ OnAction(i); });
    if (i>2)
      SetExpertRow(i);
  }

  AddBoolean(_("Use final glide mode"),
             _("Controls whether the \"final glide\" InfoBox mode should be used on \"auto\" pages."),
             settings.use_final_glide);
}

bool
InfoBoxesConfigPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  SaveValue(InfoBoxSettings::MAX_PANELS, ProfileKeys::UseFinalGlideDisplayMode,
            settings.use_final_glide);

  return true;
}

std::unique_ptr<Widget>
CreateInfoBoxesConfigPanel()
{
  return std::make_unique<InfoBoxesConfigPanel>();
}
