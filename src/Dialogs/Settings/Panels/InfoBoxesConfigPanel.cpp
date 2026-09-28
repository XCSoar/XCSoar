// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InfoBoxesConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "../dlgConfigInfoboxes.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Current.hpp"
#include "Profile/InfoBoxConfig.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Look/Look.hpp"

/**
 * The sets of InfoBoxes: one item per set, which opens the view
 * where its InfoBoxes are chosen.
 */
class InfoBoxesConfigPanel final : public ConfigListPanel {
  bool use_final_glide;

private:
  /** Open the view which edits the InfoBoxes of the set @p i. */
  void EditPanel(unsigned i) noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
InfoBoxesConfigPanel::EditPanel(unsigned i) noexcept
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;
  InfoBoxSettings::Panel &data = settings.panels[i];

  /* the set is stored right away, unlike the other settings, which
     wait for the dialog to be closed */
  const bool changed =
    dlgConfigInfoboxesShowModal(UIGlobals::GetMainWindow(),
                                UIGlobals::GetDialogLook(),
                                UIGlobals::GetLook().info_box,
                                settings.geometry, data,
                                i >= InfoBoxSettings::PREASSIGNED_PANELS);
  if (changed) {
    Profile::Save(Profile::map, data, i);
    Profile::Save();
    Refresh();
  }
}

void
InfoBoxesConfigPanel::LoadSettings() noexcept
{
  use_final_glide =
    CommonInterface::GetUISettings().info_boxes.use_final_glide;
}

void
InfoBoxesConfigPanel::Fill() noexcept
{
  const InfoBoxSettings &settings =
    CommonInterface::GetUISettings().info_boxes;

  AddGroup(_("InfoBox Sets"));

  /* the three sets of the automatic pages, and the others for the
     expert */
  const unsigned n = IsExpert()
    ? InfoBoxSettings::MAX_PANELS
    : InfoBoxSettings::PREASSIGNED_PANELS;

  for (unsigned i = 0; i < n; ++i)
    AddItem(gettext(settings.panels[i].name), [this, i](){ EditPanel(i); },
            {.chevron = true});

  AddGroup();

  AddToggleItem(_("Use final glide mode"),
                _("Controls whether the \"final glide\" InfoBox mode should be used on \"auto\" pages."),
                use_final_glide);
}

bool
InfoBoxesConfigPanel::Save(bool &_changed) noexcept
{
  InfoBoxSettings &settings = CommonInterface::SetUISettings().info_boxes;

  _changed |= Profile::Update(ProfileKeys::UseFinalGlideDisplayMode,
                              settings.use_final_glide, use_final_glide);
  return true;
}

std::unique_ptr<Widget>
CreateInfoBoxesConfigPanel()
{
  return std::make_unique<InfoBoxesConfigPanel>();
}
