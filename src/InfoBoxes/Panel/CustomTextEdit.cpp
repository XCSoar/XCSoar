// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CustomTextEdit.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/RowFormWidget.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxSettings.hpp"
#include "Interface.hpp"
#include "UIState.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Profile/Current.hpp"
#include "Profile/InfoBoxConfig.hpp"

/**
 * Edit the three lines of an #InfoBoxFactory::e_CustomText InfoBox
 * without leaving the map; the same lines can be edited in the
 * InfoBox set editor.
 */
class CustomTextEditPanel final : public RowFormWidget,
                                  private DataFieldListener {
  enum Controls {
    TITLE, VALUE, COMMENT
  };

  /** the InfoBox this panel was opened for */
  const unsigned id;

public:
  explicit CustomTextEditPanel(unsigned _id) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()), id(_id) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;

private:
  /* virtual methods from class DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
CustomTextEditPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                             [[maybe_unused]] const PixelRect &rc) noexcept
{
  const auto &settings = CommonInterface::GetUISettings().info_boxes;
  const unsigned panel_index = CommonInterface::GetUIState().panel_index;
  const InfoBoxCustomText &text = settings.panels[panel_index].text[id];

  AddText(_("Title"), _("The title line of this InfoBox."),
          text.title.c_str(), this);
  AddText(_("Value"), _("The value line of this InfoBox."),
          text.value.c_str(), this);
  AddText(_("Comment"), _("The comment line of this InfoBox."),
          text.comment.c_str(), this);
}

void
CustomTextEditPanel::OnModified([[maybe_unused]] DataField &df) noexcept
{
  auto &settings = CommonInterface::SetUISettings().info_boxes;
  const unsigned panel_index = CommonInterface::GetUIState().panel_index;
  InfoBoxSettings::Panel &panel = settings.panels[panel_index];
  InfoBoxCustomText &text = panel.text[id];

  bool modified = InfoBoxCustomText::AssignLine(text.title,
                                                GetValueString(TITLE));
  modified |= InfoBoxCustomText::AssignLine(text.value,
                                            GetValueString(VALUE));
  modified |= InfoBoxCustomText::AssignLine(text.comment,
                                            GetValueString(COMMENT));

  if (!modified)
    return;

  InfoBoxManager::SetDirty();
  Profile::Save(Profile::map, panel, panel_index);
}

std::unique_ptr<Widget>
LoadCustomTextEditPanel(unsigned id)
{
  return std::make_unique<CustomTextEditPanel>(id);
}
