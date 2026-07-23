// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RASPDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Weather/Rasp/Configured.hpp"
#include "Weather/Rasp/FieldControls.hpp"
#include "Weather/Rasp/RaspStore.hpp"
#include "Weather/MapOverlay/ControlsWidget.hpp"
#include "Weather/Settings.hpp"
#include "WeatherOverlayDraft.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Interface.hpp"
#include "PageSettings.hpp"
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
#ifdef HAVE_DOWNLOAD_MANAGER
    AUTO_UPDATE,
    UPDATE_BUTTON,
    SPACER_AFTER_UPDATE,
#endif
    LAYER,
    TIME,
    APPLY_TO_PAGE,
    ADD_PAGE,
    SPACER_AFTER_ADD,
  };

  std::shared_ptr<RaspStore> rasp;
  WeatherOverlayDraft::State overlay;

#ifdef HAVE_DOWNLOAD_MANAGER
  Button *update_button = nullptr;
#endif
  Button *apply_to_page_button = nullptr;
  Button *add_page_button = nullptr;

  static RASPSettingsPanel *active;

  void ReloadRasp();
  void UpdateModifiedDisplay();
  void UpdateLayerControl();
  void UpdateTimeControl() noexcept;
  void RefreshPageSection() noexcept;
  void SyncUpdateButtonEnabled() noexcept;
  void UpdateClicked();
  void OnLayerModified() noexcept;
  void ApplyToPageClicked() noexcept;
  void AddPageClicked() noexcept;
  bool EditTime(DataField &df) noexcept;

  static bool EditTimeCallback(const char *caption, DataField &df,
                               const char *help_text) noexcept;

public:
  explicit RASPSettingsPanel(std::shared_ptr<RaspStore> &&_rasp) noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()),
     rasp(std::move(_rasp)) {}

  ~RASPSettingsPanel() noexcept override {
    if (active == this)
      active = nullptr;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;
};

RASPSettingsPanel *RASPSettingsPanel::active = nullptr;

void
RASPSettingsPanel::ReloadRasp()
{
  rasp = LoadConfiguredRasp(false);
  DataGlobals::SetRasp(rasp);
  RaspFileChanged = true;
  Profile::Save();
  UpdateModifiedDisplay();
  RefreshPageSection();
  SyncUpdateButtonEnabled();
  WeatherMapOverlay::RefreshControlsLabels();
}

void
RASPSettingsPanel::UpdateModifiedDisplay()
{
  StaticString<32> buffer;
  buffer.clear();

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
    df.SetValue(-1);
    overlay.draft.rasp_field = -1;
    control.SetEnabled(false);
    control.RefreshDisplay();
    return;
  }

  Rasp::FieldChoicesOptions options;
  options.include_none = true;
  Rasp::FillFieldChoices(df, rasp.get(), options);

  if (overlay.draft.rasp_field < 0 ||
      unsigned(overlay.draft.rasp_field) >= rasp->GetItemCount())
    overlay.draft.rasp_field = -1;

  df.SetValue(overlay.draft.rasp_field);
  control.SetEnabled(true);
  control.RefreshDisplay();
}

void
RASPSettingsPanel::UpdateTimeControl() noexcept
{
  StaticString<64> label;
  Rasp::FormatTimeLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(TIME), label.c_str(),
                                    overlay.draft.rasp_field >= 0);
}

void
RASPSettingsPanel::RefreshPageSection() noexcept
{
  overlay.Load(PageLayout::Overlay::RASP);
  UpdateLayerControl();
  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
RASPSettingsPanel::ApplyToPageClicked() noexcept
{
  if (!overlay.ApplyIfDirty())
    return;

  UpdateLayerControl();
  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
RASPSettingsPanel::AddPageClicked() noexcept
{
  overlay.AddPage(apply_to_page_button, add_page_button);
}

bool
RASPSettingsPanel::EditTime([[maybe_unused]] DataField &df) noexcept
{
  if (!Rasp::EditTimeOnLayout(overlay.draft))
    return true;

  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
  return true;
}

bool
RASPSettingsPanel::EditTimeCallback([[maybe_unused]] const char *caption,
                                    DataField &df,
                                    [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditTime(df) : false;
}

void
RASPSettingsPanel::OnLayerModified() noexcept
{
  auto &df = (DataFieldEnum &)*GetControl(LAYER).GetDataField();
  overlay.draft.rasp_field = df.GetValue();
  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
RASPSettingsPanel::SyncUpdateButtonEnabled() noexcept
{
#ifdef HAVE_DOWNLOAD_MANAGER
  if (update_button == nullptr)
    return;

  /* Manual Update stays available even when Auto update is on
     (WeatherSettings::rasp.auto_update), but needs a file selected. */
  update_button->SetEnabled(Net::DownloadManager::IsAvailable() &&
                            !GetValueFile(FILE).empty());
#endif
}

void
RASPSettingsPanel::UpdateClicked()
{
#ifdef HAVE_DOWNLOAD_MANAGER
  if (GetValueFile(FILE).empty())
    return;

  RequestConfiguredRaspUpdate();
#endif
}

void
RASPSettingsPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  active = this;

  const auto &settings =
    CommonInterface::GetComputerSettings().weather;

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

#ifdef HAVE_DOWNLOAD_MANAGER
  AddBoolean(_("Auto update"),
             _("Automatically download a newer RASP file when the "
               "configured forecast is missing or out of date."),
             settings.rasp.auto_update);
  GetControl(AUTO_UPDATE).GetDataField()->SetOnModified([this]{
    auto &weather = CommonInterface::SetComputerSettings().weather;
    if (SaveValue(AUTO_UPDATE, ProfileKeys::RaspAutoUpdate,
                  weather.rasp.auto_update))
      Profile::Save();
    SyncUpdateButtonEnabled();
    if (weather.rasp.auto_update)
      RequestConfiguredRaspUpdateIfOutOfDate();
  });
  if (!Net::DownloadManager::IsAvailable())
    SetRowEnabled(AUTO_UPDATE, false);

  update_button = AddButton(_("Update"), [this]{ UpdateClicked(); });
  SyncUpdateButtonEnabled();
  AddSpacer();
#endif

  auto *layer = AddEnum(_("Layer"),
                        _("RASP weather layer for the current map page. "
                          "Use Apply to page to commit changes."));
  layer->GetDataField()->SetOnModified([this]{
    OnLayerModified();
  });

  auto *time = AddEnum(_("Time"),
                       _("Forecast time for the current map page. "
                         "Opens the same picker as the weather controls "
                         "(Auto, Now, or a fixed quarter-hour slot)."));
  time->SetEditCallback(EditTimeCallback);

  apply_to_page_button = AddButton(_("Apply to page"), [this]{
    ApplyToPageClicked();
  });
  add_page_button = AddButton(_("Add page"), [this]{
    AddPageClicked();
  });
  RefreshPageSection();
  AddSpacer();

  AddButton(_("Pages setup"), [this]{
    WeatherOverlayDraft::OpenPagesConfig();
    RefreshPageSection();
  });
}

void
RASPSettingsPanel::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);
  RefreshPageSection();
  SyncUpdateButtonEnabled();
}

bool
RASPSettingsPanel::Save([[maybe_unused]] bool &_changed) noexcept
{
#ifdef HAVE_DOWNLOAD_MANAGER
  auto &weather = CommonInterface::SetComputerSettings().weather;
  _changed |= SaveValue(AUTO_UPDATE, ProfileKeys::RaspAutoUpdate,
                        weather.rasp.auto_update);
#endif
  return true;
}

std::unique_ptr<Widget>
CreateRaspWidget() noexcept
{
  auto rasp = DataGlobals::GetRasp();
  return std::make_unique<RASPSettingsPanel>(std::move(rasp));
}
