// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/Edit.hpp"
#include "Repository/FileType.hpp"
#include "DataGlobals.hpp"
#include "Interface.hpp"
#include "Message.hpp"
#include "PageActions.hpp"
#include "PageSettings.hpp"
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
  : public RowFormWidget
  , private DataFieldListener
{
  enum Controls {
    FILE,
    MODIFIED,
    LAYER,
  };

  std::shared_ptr<RaspStore> rasp;
  bool suppress_layer_activation = false;

#ifdef HAVE_DOWNLOAD_MANAGER
  Button *update_button = nullptr;
#endif

  void ReloadRasp();
  void UpdateModifiedDisplay();
  void UpdateClicked();
  void FillLayerControl() noexcept;
  void UpdateLayerControls() noexcept;
  void ActivateLayer(int field) noexcept;

  void OnModified(DataField &df) noexcept override;

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
  FillLayerControl();
  UpdateLayerControls();
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
RASPSettingsPanel::UpdateClicked()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  RequestConfiguredRaspUpdate();
#endif
}

void
RASPSettingsPanel::FillLayerControl() noexcept
{
  auto &control = GetControl(LAYER);
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
RASPSettingsPanel::UpdateLayerControls() noexcept
{
  rasp = DataGlobals::GetRasp();
  const bool available = rasp != nullptr && rasp->GetItemCount() > 0;
  SetRowEnabled(LAYER, available);
}

void
RASPSettingsPanel::ActivateLayer(int field) noexcept
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
RASPSettingsPanel::OnModified(DataField &df) noexcept
{
  if (suppress_layer_activation || &df != &GetDataField(LAYER))
    return;

  ActivateLayer(((const DataFieldEnum &)df).GetValue());
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

  AddEnum(_("RASP layer"),
          _("RASP weather layer to display on this map page."),
          this);
  GetControl(LAYER).GetDataField()->EnableItemHelp(true);
  FillLayerControl();

#ifdef HAVE_DOWNLOAD_MANAGER
  update_button = AddButton(_("Update"), [this]{ UpdateClicked(); });
  if (!Net::DownloadManager::IsAvailable())
    update_button->SetEnabled(false);
#endif

  UpdateLayerControls();
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
