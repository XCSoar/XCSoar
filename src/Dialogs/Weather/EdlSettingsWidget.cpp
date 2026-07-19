// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlSettingsWidget.hpp"

#include "Dialogs/Message.hpp"
#include "WeatherOverlayDraft.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Form/Button.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/Edit.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "PageSettings.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "Weather/Features.hpp"
#include "Weather/Settings.hpp"
#include "Weather/EDL/StateController.hpp"
#include "Weather/EDL/TileStore.hpp"
#ifdef HAVE_EDL
#include "Weather/EDL/FieldControls.hpp"
#include "Weather/EDL/Glue.hpp"
#include "Weather/EDL/DownloadGlue.hpp"
#endif
#include "Widget/RowFormWidget.hpp"
#include "util/StaticString.hxx"

#include <memory>
#include <vector>

namespace {

unsigned
SelectedCachedDayIndex(const std::vector<EDL::CachedDay> &days) noexcept
{
  if (days.empty())
    return 0;

  const auto current_day = EDL::GetForecastTime().AtMidnight();
  for (unsigned i = 0; i < days.size(); ++i)
    if (days[i].day == current_day)
      return i;

  return 0;
}

StaticString<40>
FormatCachedDayLabel(const EDL::CachedDay &day) noexcept
{
  StaticString<40> label;
  label.Format("%04u-%02u-%02u (%s, %u)",
               day.day.year, day.day.month, day.day.day,
               day.IsComplete() ? _("Complete") : _("Partial"),
               day.file_count);
  return label;
}

} // namespace

class EdlSettingsWidget final
  : public RowFormWidget
#ifdef HAVE_EDL
  , private EDL::DownloadListener
#endif
{
  enum Controls {
    CACHED_DAY,
    AUTO_UPDATE,
#ifdef HAVE_HTTP
    PRECACHE_DAY,
#endif
    CLEAN_OTHER_DAYS,
    SPACER_AFTER_CACHE,
#ifdef HAVE_EDL
    TIME,
    LEVEL,
    APPLY_TO_PAGE,
    ADD_PAGE,
#endif
    SPACER_AFTER_ADD,
  };

  std::vector<EDL::CachedDay> cached_days;

  Button *precache_day_button = nullptr;
  Button *clean_other_days_button = nullptr;
#ifdef HAVE_EDL
  Button *apply_to_page_button = nullptr;
  Button *add_page_button = nullptr;
  EDL::DownloadGlue *edl_listener_glue = nullptr;
  WeatherOverlayDraft::State overlay;

  static EdlSettingsWidget *active;
#endif

public:
  EdlSettingsWidget() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  ~EdlSettingsWidget() noexcept override {
#ifdef HAVE_EDL
    UnregisterEdlDownloadListener();
    if (active == this)
      active = nullptr;
#endif
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  void Unprepare() noexcept override;
  bool Save(bool &changed) noexcept override;

private:
#ifdef HAVE_EDL
  void UnregisterEdlDownloadListener() noexcept;
  void UpdateTimeControl() noexcept;
  void UpdateLevelControl() noexcept;
  void RefreshPageSection() noexcept;
  void ApplyToPageClicked() noexcept;
  void AddPageClicked() noexcept;
  bool EditTime(DataField &df) noexcept;
  bool EditLevel(DataField &df) noexcept;

  static bool EditTimeCallback(const char *caption, DataField &df,
                               const char *help_text) noexcept;
  static bool EditLevelCallback(const char *caption, DataField &df,
                                const char *help_text) noexcept;
#endif
  void SyncPrecacheButtonEnabled() noexcept;
  void RefreshControls();
  void PrecacheDay();
  void CleanOtherDays();

#ifdef HAVE_EDL
  void OnDownloadFinished(const EDL::DownloadNotification &notification) noexcept override;
#endif
};

#ifdef HAVE_EDL
EdlSettingsWidget *EdlSettingsWidget::active = nullptr;

void
EdlSettingsWidget::UnregisterEdlDownloadListener() noexcept
{
  if (edl_listener_glue == nullptr)
    return;

  edl_listener_glue->RemoveListener(*this);
  edl_listener_glue = nullptr;
}

void
EdlSettingsWidget::UpdateTimeControl() noexcept
{
  StaticString<64> label;
  EDL::FormatTimeLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(TIME), label.c_str(), true);
}

void
EdlSettingsWidget::UpdateLevelControl() noexcept
{
  StaticString<64> label;
  EDL::FormatLevelLabelForPage(label, overlay.draft);
  WeatherOverlayDraft::SetAxisLabel(GetControl(LEVEL), label.c_str(), true);
}

void
EdlSettingsWidget::RefreshPageSection() noexcept
{
  overlay.Load(PageLayout::Overlay::EDL);
  UpdateTimeControl();
  UpdateLevelControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
EdlSettingsWidget::ApplyToPageClicked() noexcept
{
  if (!overlay.ApplyIfDirty())
    return;

  UpdateTimeControl();
  UpdateLevelControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
}

void
EdlSettingsWidget::AddPageClicked() noexcept
{
  overlay.AddPage(apply_to_page_button, add_page_button);
}

bool
EdlSettingsWidget::EditTime([[maybe_unused]] DataField &df) noexcept
{
  if (!EDL::EditTimeOnLayout(overlay.draft))
    return true;

  UpdateTimeControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
  return true;
}

bool
EdlSettingsWidget::EditLevel([[maybe_unused]] DataField &df) noexcept
{
  const auto result = EDL::EditLevelOnLayout(overlay.draft, false);
  if (result == EDL::LevelPickerResult::OPEN_SETUP)
    return false;
  if (result != EDL::LevelPickerResult::CHANGED)
    return true;

  UpdateLevelControl();
  overlay.SyncButtons(apply_to_page_button, add_page_button);
  return true;
}

bool
EdlSettingsWidget::EditTimeCallback([[maybe_unused]] const char *caption,
                                    DataField &df,
                                    [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditTime(df) : false;
}

bool
EdlSettingsWidget::EditLevelCallback([[maybe_unused]] const char *caption,
                                     DataField &df,
                                     [[maybe_unused]] const char *help_text) noexcept
{
  return active != nullptr ? active->EditLevel(df) : false;
}
#endif

void
EdlSettingsWidget::RefreshControls()
{
  auto &control = GetControl(CACHED_DAY);
  auto &df = (DataFieldEnum &)*control.GetDataField();
  df.ClearChoices();

  try {
    cached_days = EDL::ListDownloadedDays();
  } catch (...) {
    cached_days.clear();
  }

  if (cached_days.empty()) {
    df.AddChoice(unsigned(-1), _("None"));
    df.SetValue(unsigned(-1));
    control.SetEnabled(false);
    if (clean_other_days_button != nullptr)
      clean_other_days_button->SetEnabled(false);
  } else {
    control.SetEnabled(true);
    if (clean_other_days_button != nullptr)
      clean_other_days_button->SetEnabled(true);

    const unsigned selected_index =
      SelectedCachedDayIndex(cached_days);
    for (unsigned i = 0; i < cached_days.size(); ++i)
      df.AddChoice(i, FormatCachedDayLabel(cached_days[i]).c_str());

    df.SetValue(selected_index);
  }

  control.RefreshDisplay();

  SyncPrecacheButtonEnabled();

#ifdef HAVE_EDL
  RefreshPageSection();
#endif
}

void
EdlSettingsWidget::SyncPrecacheButtonEnabled() noexcept
{
  if (precache_day_button == nullptr)
    return;

  const bool can_precache =
#ifdef HAVE_EDL
    net_components != nullptr && net_components->edl != nullptr &&
    !GetValueBoolean(AUTO_UPDATE);
#else
    false;
#endif
  precache_day_button->SetEnabled(can_precache);
}

void
EdlSettingsWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

#ifdef HAVE_EDL
  active = this;
#endif

  const auto &settings =
    CommonInterface::GetComputerSettings().weather;

  AddEnum(_("Cached day"), nullptr);

  AddBoolean(_("Auto update"),
             _("Automatically download missing EDL overlay tiles when "
               "an EDL page is opened or the forecast time/level changes. "
               "When Auto update is on, the Precache day button is "
               "disabled."),
             settings.edl.auto_update);
  GetControl(AUTO_UPDATE).GetDataField()->SetOnModified([this]{
    auto &weather = CommonInterface::SetComputerSettings().weather;
    if (SaveValue(AUTO_UPDATE, ProfileKeys::EdlAutoUpdate,
                  weather.edl.auto_update))
      Profile::Save();
    SyncPrecacheButtonEnabled();
  });

#ifdef HAVE_HTTP
  precache_day_button = AddButton(_("Precache day"), [this]{ PrecacheDay(); });
  SyncPrecacheButtonEnabled();
#endif

  clean_other_days_button = AddButton(_("Clean other days"),
                                      [this]{ CleanOtherDays(); });
  AddSpacer();

#ifdef HAVE_EDL
  auto *time = AddEnum(_("Time"),
                       _("Forecast time for the current map page. "
                         "Opens the same picker as the weather controls "
                         "(Auto, Now, or a UTC hour)."));
  time->SetEditCallback(EditTimeCallback);

  auto *level = AddEnum(_("Level"),
                        _("Pressure level / altitude band for the current "
                          "map page. Opens the same picker as the weather "
                          "controls."));
  level->SetEditCallback(EditLevelCallback);

  apply_to_page_button = AddButton(_("Apply to page"), [this]{
    ApplyToPageClicked();
  });
  add_page_button = AddButton(_("Add page"), [this]{
    AddPageClicked();
  });
#endif

  AddSpacer();

  AddButton(_("Pages setup"), [this]{
    WeatherOverlayDraft::OpenPagesConfig();
#ifdef HAVE_EDL
    RefreshPageSection();
#endif
  });
}

void
EdlSettingsWidget::Show(const PixelRect &rc) noexcept
{
  RowFormWidget::Show(rc);

  RefreshControls();

#ifdef HAVE_EDL
  if (net_components != nullptr && net_components->edl != nullptr) {
    edl_listener_glue = net_components->edl.get();
    edl_listener_glue->AddListener(*this);
  }
#endif
}

void
EdlSettingsWidget::Hide() noexcept
{
#ifdef HAVE_EDL
  UnregisterEdlDownloadListener();
#endif

  WindowWidget::Hide();
}

void
EdlSettingsWidget::Unprepare() noexcept
{
#ifdef HAVE_EDL
  if (active == this)
    active = nullptr;
#endif
  precache_day_button = nullptr;
  clean_other_days_button = nullptr;
  cached_days.clear();
  RowFormWidget::Unprepare();
}

bool
EdlSettingsWidget::Save(bool &_changed) noexcept
{
  auto &weather = CommonInterface::SetComputerSettings().weather;
  _changed |= SaveValue(AUTO_UPDATE, ProfileKeys::EdlAutoUpdate,
                        weather.edl.auto_update);
  return true;
}

void
EdlSettingsWidget::PrecacheDay()
{
#if !defined(HAVE_HTTP) || !defined(HAVE_EDL)
  ShowMessageBox(_("HTTP support is not available in this build."),
                 _("Weather"), MB_OK);
#else
  EDL::EnsureInitialised();
  EDL::RequestPrecacheDay(EDL::GetForecastTime());
#endif
}

#ifdef HAVE_EDL
void
EdlSettingsWidget::OnDownloadFinished(
  const EDL::DownloadNotification &) noexcept
{
  RefreshControls();
}
#endif

void
EdlSettingsWidget::CleanOtherDays()
{
  if (cached_days.empty())
    return;

  const auto selected_index = GetValueEnum(CACHED_DAY);
  if (selected_index >= cached_days.size())
    return;

  StaticString<96> message;
  message.Format(_("Keep only %04u-%02u-%02u and delete the other cached days?"),
                 cached_days[selected_index].day.year,
                 cached_days[selected_index].day.month,
                 cached_days[selected_index].day.day);
  if (ShowMessageBox(message, _("Weather"), MB_YESNO) != IDYES)
    return;

  const unsigned deleted =
    EDL::DeleteOtherDownloadedDays(cached_days[selected_index].day);
  RefreshControls();

  StaticString<64> result;
  result.Format(_("Deleted %u cached files."), deleted);
  ShowMessageBox(result, _("Weather"), MB_OK);
}

std::unique_ptr<Widget>
CreateEdlSettingsWidget() noexcept
{
  return std::make_unique<EdlSettingsWidget>();
}
