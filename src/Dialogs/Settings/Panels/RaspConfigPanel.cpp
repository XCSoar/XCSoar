// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RaspConfigPanel.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Repository/FileType.hpp"
#include "UtilsSettings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "DataGlobals.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Message.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
#include "UIGlobals.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"

enum ControlIndex {
  RaspFile,
  RaspLayer,
};

class RaspConfigPanel final
  : public RowFormWidget
  , private DataFieldListener
{
  std::shared_ptr<RaspStore> rasp;
  bool suppress_layer_activation = false;

  void ReloadRasp();
  void FillLayerControl() noexcept;
  void UpdateLayerControls() noexcept;
  void ActivateLayer(int field) noexcept;

  void OnModified(DataField &df) noexcept override;

public:
  RaspConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RaspConfigPanel::ReloadRasp()
{
  rasp = LoadConfiguredRasp(false);
  DataGlobals::SetRasp(rasp);
  RaspFileChanged = true;
  FillLayerControl();
  UpdateLayerControls();
}

void
RaspConfigPanel::FillLayerControl() noexcept
{
  auto &control = GetControl(RaspLayer);
  auto &df = (DataFieldEnum &)*control.GetDataField();

  suppress_layer_activation = true;

  rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0) {
    df.ClearChoices();
    df.AddChoice(-1, _("No RASP file loaded"));
    df.SetValue(-1);
    control.RefreshDisplay();
    suppress_layer_activation = false;
    return;
  }

  Rasp::FillFieldChoices(df, rasp.get(), {.include_none = true});
  df.SetValue(-1);

  control.RefreshDisplay();
  suppress_layer_activation = false;
}

void
RaspConfigPanel::UpdateLayerControls() noexcept
{
  rasp = DataGlobals::GetRasp();
  const bool available = rasp != nullptr && rasp->GetItemCount() > 0;
  SetRowEnabled(RaspLayer, available);
}

void
RaspConfigPanel::ActivateLayer(int field) noexcept
{
  rasp = DataGlobals::GetRasp();
  if (rasp == nullptr || rasp->GetItemCount() == 0)
    return;

  if (field < 0 || unsigned(field) >= rasp->GetItemCount())
    return;

  const unsigned page_index = PageActions::EnsureWeatherOverlayPage(
    PageLayout::Overlay::RASP, field);
  if (page_index >= PageSettings::MAX_PAGES) {
    Message::AddMessage(_("No free map page for weather overlay."));
    return;
  }

  PageActions::GoToPage(page_index);
}

void
RaspConfigPanel::OnModified(DataField &df) noexcept
{
  if (suppress_layer_activation || &df != &GetDataField(RaspLayer))
    return;

  ActivateLayer(((const DataFieldEnum &)df).GetValue());
}

void
RaspConfigPanel::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  WndProperty *wp = AddFile("RASP",
                            _("Regional Atmospheric Soaring Prediction file providing "
                              "weather forecasts for soaring. Displays color-coded map "
                              "overlays for thermal strength, boundary layer winds, "
                              "cloud cover, and other soaring-relevant parameters at "
                              "various forecast times throughout the day."),
                            ProfileKeys::RaspFile,
                            GetFileTypePatterns(FileType::RASP),
                            FileType::RASP);
  wp->GetDataField()->SetOnModified([this]{
    if (SaveValueFileReader(RaspFile, ProfileKeys::RaspFile))
      ReloadRasp();
  });

  AddEnum(_("RASP layer"),
          _("RASP weather layer to display on this map page."),
          this);
  GetControl(RaspLayer).GetDataField()->EnableItemHelp(true);
  FillLayerControl();
  UpdateLayerControls();
}

bool
RaspConfigPanel::Save(bool &_changed) noexcept
{
  RaspFileChanged = SaveValueFileReader(RaspFile, ProfileKeys::RaspFile);

  _changed |= RaspFileChanged;

  if (RaspFileChanged)
    ReloadRasp();

  return true;
}

std::unique_ptr<Widget>
CreateRaspConfigPanel()
{
  return std::make_unique<RaspConfigPanel>();
}
