// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "OverlayPageActions.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Repository/FileType.hpp"
#include "DataGlobals.hpp"
#include "UIGlobals.hpp"
#include "UtilsSettings.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "net/http/Features.hpp"
#ifdef HAVE_DOWNLOAD_MANAGER
#include "Weather/Rasp/DownloadGlue.hpp"
#include "net/http/DownloadManager.hpp"
#endif

class RASPSettingsPanel final
  : public RowFormWidget {

  enum Controls {
    FILE,
    MODIFIED,
    LAYER,
  };

  std::shared_ptr<RaspStore> rasp;
  int selected_field = -1;

#ifdef HAVE_DOWNLOAD_MANAGER
  Button *update_button = nullptr;
#endif

  void ReloadRasp();
  void UpdateModifiedDisplay();
  void UpdateLayerControl();
  int GetPlacementFieldIndex() const noexcept;
  void UpdateClicked();
  void AddToCurrentClicked();
  void AddToNewPageClicked();

public:
  explicit RASPSettingsPanel(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     rasp(std::move(_rasp)) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

void
RASPSettingsPanel::ReloadRasp()
{
  rasp = LoadConfiguredRasp(false);
  DataGlobals::SetRasp(rasp);
  RaspFileChanged = true;
  Profile::Save();
  UpdateModifiedDisplay();
  UpdateLayerControl();
}

void
RASPSettingsPanel::UpdateModifiedDisplay()
{
  StaticString<32> buffer;

  if (rasp != nullptr) {
    const BrokenDateTime modified = rasp->GetFileModifiedTime();
    if (modified.IsPlausible()) {
      buffer.Format("%04u-%02u-%02u %02u:%02u",
                      modified.year, modified.month, modified.day,
                      modified.hour, modified.minute);
    }
  }

  if (buffer.empty())
    buffer = _("Unknown");

  SetText(MODIFIED, buffer.c_str());
}

void
RASPSettingsPanel::UpdateLayerControl()
{
  auto &control = GetControl(LAYER);
  auto &df = (DataFieldEnum &)*control.GetDataField();
  df.ClearChoices();

  if (rasp == nullptr || rasp->GetItemCount() == 0) {
    df.AddChoice(-1, "none", _("None"), nullptr);
    df.AddChoice(-2, _("No RASP file loaded"));
    df.SetValue(-1);
    selected_field = -1;
    control.SetEnabled(true);
    control.RefreshDisplay();
    return;
  }

  Rasp::FieldChoicesOptions options;
  options.include_none = true;
  Rasp::FillFieldChoices(df, rasp.get(), options);

  if (selected_field < 0 || unsigned(selected_field) >= rasp->GetItemCount())
    selected_field = 0;

  df.SetValue(selected_field);
  control.SetEnabled(true);
  control.RefreshDisplay();
}

void
RASPSettingsPanel::UpdateClicked()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  RequestConfiguredRaspUpdate();
#endif
}

int
RASPSettingsPanel::GetPlacementFieldIndex() const noexcept
{
  return selected_field >= 0
    ? selected_field
    : Rasp::GetActiveFieldIndex();
}

void
RASPSettingsPanel::AddToCurrentClicked()
{
  if (GetPlacementFieldIndex() < 0) {
    WeatherDialogOverlayActions::AddOverlayToCurrentPage(
      PageLayout::Overlay::NONE);
    return;
  }

  WeatherDialogOverlayActions::AddOverlayToCurrentPage(
    PageLayout::Overlay::RASP, GetPlacementFieldIndex());
}

void
RASPSettingsPanel::AddToNewPageClicked()
{
  if (GetPlacementFieldIndex() < 0) {
    WeatherDialogOverlayActions::AddOverlayToNewPage(
      PageLayout::Overlay::NONE);
    return;
  }

  WeatherDialogOverlayActions::AddOverlayToNewPage(
    PageLayout::Overlay::RASP, GetPlacementFieldIndex());
}

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  WndProperty *wp = AddFile(_("File"), nullptr,
                            ProfileKeys::RaspFile,
                            GetFileTypePatterns(FileType::RASP),
                            FileType::RASP);
  wp->GetDataField()->SetOnModified([this]{
    if (SaveValueFileReader(FILE, ProfileKeys::RaspFile)) {
      ReloadRasp();
      GetControl(FILE).RefreshDisplay();
    }
  });

  AddReadOnly(_("Modified"),
              _("Local date and time of the selected RASP file."));
  UpdateModifiedDisplay();

  auto *layer = AddEnum(_("Layer"),
                        _("RASP layer used when adding the overlay to pages."));
  layer->GetDataField()->SetOnModified([this, layer]{
    selected_field = ((DataFieldEnum &)*layer->GetDataField()).GetValue();
  });
  UpdateLayerControl();

#ifdef HAVE_DOWNLOAD_MANAGER
  update_button = AddButton(_("Update"), [this]{ UpdateClicked(); });
  if (!Net::DownloadManager::IsAvailable())
    update_button->SetEnabled(false);
#endif

  AddButton(_("Add to page"), [this]{ AddToCurrentClicked(); });
  AddButton(_("Add new page"), [this]{ AddToNewPageClicked(); });
}

bool
RASPSettingsPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RASPSettingsPanel>(std::move(rasp));
}
