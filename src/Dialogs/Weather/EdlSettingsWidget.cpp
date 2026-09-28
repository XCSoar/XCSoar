// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EdlSettingsWidget.hpp"

#include "Dialogs/GroupedListPicker.hpp"
#include "Dialogs/Message.hpp"
#include "WeatherOverlayDraft.hpp"
#include "Components.hpp"
#include "NetComponents.hpp"
#include "Interface.hpp"
#include "UIState.hpp"
#include "Language/Language.hpp"
#include "Language/FormatText.hpp"
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
#include "Widget/GroupedListWidget.hpp"
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
               day.IsComplete()
                 ? C_("Status", "Complete")
                 : C_("Status", "Partial"),
               day.file_count);
  return label;
}

} // namespace

/**
 * The EDL weather page: which day is cached, whether tiles download
 * by themselves, and the time and level of the page the map shows.
 */
class EdlSettingsWidget final
  : public GroupedListWidget
#ifdef HAVE_EDL
  , private EDL::DownloadListener
#endif
{
  std::vector<EDL::CachedDay> cached_days;
  unsigned selected_day = 0;

  bool auto_update = false;

#ifdef HAVE_EDL
  EDL::DownloadGlue *edl_listener_glue = nullptr;
  WeatherOverlayDraft::State overlay;
#endif

public:
  EdlSettingsWidget() noexcept
    :GroupedListWidget(UIGlobals::GetDialogLook()) {}

  ~EdlSettingsWidget() noexcept override {
#ifdef HAVE_EDL
    UnregisterEdlDownloadListener();
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
#endif
  void ReloadCachedDays() noexcept;
  void Refresh() noexcept;
  void Fill() noexcept;
  void PickCachedDay() noexcept;
  void PrecacheDay();
  void CleanOtherDays();

#ifdef HAVE_EDL
  void OnDownloadFinished(const EDL::DownloadNotification &) noexcept override;
#endif
};

#ifdef HAVE_EDL
void
EdlSettingsWidget::UnregisterEdlDownloadListener() noexcept
{
  if (edl_listener_glue == nullptr)
    return;

  edl_listener_glue->RemoveListener(*this);
  edl_listener_glue = nullptr;
}
#endif

void
EdlSettingsWidget::ReloadCachedDays() noexcept
{
  try {
    cached_days = EDL::ListDownloadedDays();
  } catch (...) {
    cached_days.clear();
  }

  selected_day = SelectedCachedDayIndex(cached_days);
}

void
EdlSettingsWidget::Refresh() noexcept
{
  ReloadCachedDays();
  Clear();
  Fill();

  /* the day, the time and the level are filled by their callbacks.
     A layout before that leaves them blank */
  UpdateValues();
  UpdateLayout();
}

void
EdlSettingsWidget::PickCachedDay() noexcept
{
  if (cached_days.empty())
    return;

  std::vector<StaticString<40>> labels;
  std::vector<PickerChoice> choices;
  labels.reserve(cached_days.size());
  choices.reserve(cached_days.size());

  for (const auto &day : cached_days) {
    labels.push_back(FormatCachedDayLabel(day));
    choices.push_back({labels.back().c_str()});
  }

  const int current = selected_day < cached_days.size()
    ? (int)selected_day : -1;
  const int picked = PickChoice(C_("Setting", "Cached day"), nullptr,
                                choices, current);
  if (picked < 0)
    return;

  selected_day = (unsigned)picked;
  UpdateValues();
}

void
EdlSettingsWidget::Fill() noexcept
{
  AddGroup();

  AddValue(C_("Setting", "Cached day"), nullptr,
           [this](ValueState &state) {
             if (cached_days.empty() ||
                 selected_day >= cached_days.size()) {
               state.text = _("None");
               state.disabled = true;
               return;
             }

             const auto label =
               FormatCachedDayLabel(cached_days[selected_day]);
             state.text = label.c_str();
           },
           [this]{ PickCachedDay(); });

  const unsigned auto_update_item = GetItemCount();
  AddItem(C_("Setting", "Auto update"), [this, auto_update_item]{
    auto_update = IsItemChecked(auto_update_item);

    auto &weather = CommonInterface::SetComputerSettings().weather;
    if (Profile::Update(ProfileKeys::EdlAutoUpdate,
                        weather.edl.auto_update, auto_update))
      Profile::Save();

    /* Precache day is off while this switch is on */
    Refresh();
  }, {.toggle = true, .checked = auto_update,
      .help = _("Automatically download missing EDL overlay tiles when "
                "an EDL page is opened or the forecast time/level changes. "
                "When Auto update is on, the Precache day button is "
                "disabled.")});

#ifdef HAVE_HTTP
  const bool can_precache =
#ifdef HAVE_EDL
    !auto_update &&
    net_components != nullptr && net_components->edl != nullptr;
#else
    false;
#endif
  AddButton(C_("Button", "Precache day"), [this]{ PrecacheDay(); },
            {.disabled = !can_precache});
#endif

  AddButton(C_("Button", "Clean other days"), [this]{ CleanOtherDays(); },
            {.disabled = cached_days.empty()});

#ifdef HAVE_EDL
  const auto &ui_state = CommonInterface::GetUIState();
  const auto &ui_settings = CommonInterface::GetUISettings();
  const unsigned page_index = ui_state.pages.current_index;
  const PageLayout &page = ui_settings.pages.pages[page_index];

  StaticString<64> title_buffer;
  const char *title =
    page.MakeTitle(ui_settings.info_boxes,
                   std::span{title_buffer.data(), title_buffer.capacity()});

  StaticString<128> caption;
  caption.Format("%s %u: %s", _("Page"), page_index + 1, title);
  AddGroup(caption);

  AddValue(C_("Weather control", "Time"),
           _("Forecast time for the current map page. "
             "Opens the same picker as the weather controls "
             "(Auto, Now, or a UTC hour)."),
           [this](ValueState &state) {
             StaticString<64> label;
             EDL::FormatTimeLabelForPage(label, overlay.draft);
             state.text = label.c_str();
           },
           [this]{
             if (EDL::EditTimeOnLayout(overlay.draft))
               Refresh();
           });

  AddValue(C_("Weather control", "Level"),
           _("Pressure level / altitude band for the current "
             "map page. Opens the same picker as the weather "
             "controls."),
           [this](ValueState &state) {
             StaticString<64> label;
             EDL::FormatLevelLabelForPage(label, overlay.draft);
             state.text = label.c_str();
           },
           [this]{
             const auto result =
               EDL::EditLevelOnLayout(overlay.draft, false);
             if (result == EDL::LevelPickerResult::CHANGED)
               Refresh();
           });

  AddButtonRow({
    {C_("Button", "Apply to page"), [this]{
      if (overlay.ApplyIfDirty())
        Refresh();
    }, !overlay.IsDirty()},
    {C_("Button", "Add page"), [this]{
      overlay.AddPage(nullptr, nullptr);
      Refresh();
    }, !overlay.CanAddPage()},
  });
#endif

  AddGroup();
  AddButton(C_("Button", "Pages setup"), [this]{
    WeatherOverlayDraft::OpenPagesConfig();
#ifdef HAVE_EDL
    overlay.Load(PageLayout::Overlay::EDL);
    Refresh();
#endif
  });
}

void
EdlSettingsWidget::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  auto_update =
    CommonInterface::GetComputerSettings().weather.edl.auto_update;

#ifdef HAVE_EDL
  overlay.Load(PageLayout::Overlay::EDL);
#endif

  ReloadCachedDays();
  Fill();
  GroupedListWidget::Prepare(parent, rc);
}

void
EdlSettingsWidget::Show(const PixelRect &rc) noexcept
{
  GroupedListWidget::Show(rc);

#ifdef HAVE_EDL
  overlay.Load(PageLayout::Overlay::EDL);
#endif
  Refresh();

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

  GroupedListWidget::Hide();
}

void
EdlSettingsWidget::Unprepare() noexcept
{
#ifdef HAVE_EDL
  UnregisterEdlDownloadListener();
#endif
  cached_days.clear();
  GroupedListWidget::Unprepare();
}

bool
EdlSettingsWidget::Save(bool &_changed) noexcept
{
  auto &weather = CommonInterface::SetComputerSettings().weather;
  if (Profile::Update(ProfileKeys::EdlAutoUpdate,
                      weather.edl.auto_update, auto_update))
    _changed = true;

  return true;
}

void
EdlSettingsWidget::PrecacheDay()
{
#if !defined(HAVE_HTTP) || !defined(HAVE_EDL)
  StaticString<128> message;
  FormatFeatureNotAvailableInThisBuild(message, C_("Setting", "HTTP support"));
  ShowMessageBox(message, _("Weather"), MB_OK);
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
  Refresh();
}
#endif

void
EdlSettingsWidget::CleanOtherDays()
{
  if (cached_days.empty() || selected_day >= cached_days.size())
    return;

  const auto &day = cached_days[selected_day].day;

  StaticString<96> message;
  message.Format(
    _("Keep only %04u-%02u-%02u and delete the other cached days?"),
    day.year, day.month, day.day);
  if (ShowMessageBox(message, _("Weather"), MB_YESNO) != IDYES)
    return;

  const unsigned deleted = EDL::DeleteOtherDownloadedDays(day);
  Refresh();

  StaticString<64> result;
  result.Format(_("Deleted %u cached files."), deleted);
  ShowMessageBox(result, _("Weather"), MB_OK);
}

std::unique_ptr<Widget>
CreateEdlSettingsWidget() noexcept
{
  return std::make_unique<EdlSettingsWidget>();
}
