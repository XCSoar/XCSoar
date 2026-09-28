// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "Dialogs/Airspace/NOTAMList.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Formatter/UserUnits.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "NOTAM/Config.hpp"
#include "NOTAM/Filter.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#include "NetComponents.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Protection.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"
#include "ui/event/Notify.hpp"
#include "util/StaticString.hxx"

#include <algorithm>
#include <exception>
#include <span>

/** the choices of a radius, in kilometres */
static constexpr unsigned NOTAM_RADIUS_STEP_KM = 10;

/**
 * The download of the NOTAMs and the filter which picks the ones the
 * map shows.  The counts below the filter items follow the NOTAMs as
 * they are loaded.
 */
class NOTAMConfigPanel final : public ConfigListPanel, NOTAMListener {
  /** the values of the page */
  NOTAMSettings settings;

  /** the largest radius of a NOTAM the map shows, in kilometres; 0
      shows all */
  unsigned max_radius_km;

  /** how many NOTAMs each filter hides, shown below its item */
  NOTAMFilter::FilterStats stats;

  /** is a download running which the Refresh button started? */
  bool loading = false;

  bool saving_for_manual_update = false;

  /** the loader reports from its thread; the page redraws in the UI
      thread */
  UI::Notify notify{[this]() { UpdateFilterCounts(); }};

private:
  /**
   * Add an item which opens the choice of a radius, one choice per
   * #NOTAM_RADIUS_STEP_KM from @p min_km; the value is in
   * kilometres.
   */
  void AddRadiusItem(const char *caption, const char *help,
                     unsigned min_km, unsigned &value_km,
                     const char *subtitle=nullptr) noexcept;

  void AddQCodesItem() noexcept;

  /** the text below a filter item: how many NOTAMs it hides */
  const char *FormatFilterCount(StaticString<64> &buffer,
                                unsigned count) const noexcept;

  void OnUpdateButton() noexcept;
  void OnListButton() noexcept;
  void UpdateFilterCounts() noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  /* virtual methods from class NOTAMListener */
  void OnNOTAMsUpdated() noexcept override;
  void OnNOTAMsLoadComplete(NOTAMLoadNotification notification)
    noexcept override;
};

void
NOTAMConfigPanel::LoadSettings() noexcept
{
  settings = CommonInterface::GetComputerSettings().airspace.notam;
  max_radius_km = (settings.max_radius_m + 500) / 1000;

  if (net_components != nullptr && net_components->notam != nullptr)
    stats = net_components->notam->GetFilterStats();
}

void
NOTAMConfigPanel::AddRadiusItem(const char *caption, const char *help,
                                unsigned min_km, unsigned &value_km,
                                const char *subtitle) noexcept
{
  AddItem(caption, [this, caption, help, min_km, &value_km](){
    constexpr unsigned max_km = MAX_NOTAM_REQUEST_RADIUS_KM;
    constexpr unsigned n = max_km / NOTAM_RADIUS_STEP_KM + 1;

    BasicStringBuffer<char, 32> captions[n];
    PickerChoice choices[n];
    unsigned count = 0;

    for (unsigned km = min_km; km <= max_km; km += NOTAM_RADIUS_STEP_KM) {
      captions[count] = FormatUserDistance(km * 1000.);
      choices[count] = {captions[count].c_str()};
      ++count;
    }

    /* the choice nearest to the value */
    const int current =
      std::clamp(((int)value_km - (int)min_km + (int)NOTAM_RADIUS_STEP_KM / 2)
                 / (int)NOTAM_RADIUS_STEP_KM,
                 0, (int)count - 1);

    const int picked = PickChoice(caption, help,
                                  std::span{choices, count}, current);
    if (picked < 0)
      return;

    const unsigned new_km = min_km + NOTAM_RADIUS_STEP_KM * picked;
    if (new_km == value_km)
      return;

    value_km = new_km;
    Refresh();
  }, {.subtitle = subtitle,
      .value = FormatUserDistance(value_km * 1000.).c_str(),
      .chevron = true});
}

void
NOTAMConfigPanel::AddQCodesItem() noexcept
{
  StaticString<64> count;

  AddItem(_("Hidden Q-Codes"), [this](){
    StaticString<256> text = settings.hidden_qcodes;
    if (!TextEntryDialog(text, _("Hidden Q-Codes")))
      return;

    settings.hidden_qcodes = text;
    Refresh();
  }, {.subtitle = FormatFilterCount(count, stats.filtered_by_qcode),
      .value = settings.hidden_qcodes.c_str(),
      .value_font = TextFont::MONO,
      .value_size = TextSize::SMALL,
      .value_all_lines = true,
      .chevron = true,
      .help = _("Space-separated Q-code prefixes to hide (e.g., QA QK QN QOA QOL).")});
}

const char *
NOTAMConfigPanel::FormatFilterCount(StaticString<64> &buffer,
                                    unsigned count) const noexcept
{
  if (loading)
    return C_("Status", "Loading...");

  buffer.Format(_("%u filtered"), count);
  return buffer.c_str();
}

void
NOTAMConfigPanel::Fill() noexcept
{
  AddHero(C_("Setting", "NOTAM"),
          _("Notice: NOTAM display is for situational awareness only "
            "and does not replace proper pre-flight NOTAM briefing."));

  AddGroup();

  AddToggleItem(_("NOTAM Support"),
                _("Enable downloading and display of NOTAMs from aviation authorities."),
                settings.enabled);

  if (!settings.enabled)
    return;

  AddItem(_("API URL"), [this](){
    StaticString<128> text = settings.api_base_url;
    if (!TextEntryDialog(text, _("API URL")))
      return;

    settings.api_base_url = text;
    Refresh();
  }, {.value = settings.api_base_url.c_str(),
      .value_below = true,
      .value_font = TextFont::MONO,
      .value_size = TextSize::SMALL,
      .value_all_lines = true,
      .chevron = true,
      .help = _("Base URL of the NOTAM proxy API. Must be configured before NOTAMs can be fetched.")});

  AddRadiusItem(_("Search Radius"),
                _("Radius around current location to fetch NOTAMs."),
                NOTAM_RADIUS_STEP_KM, settings.radius_km);

  StaticString<32> minutes;
  minutes.Format(_("%d min"), settings.refresh_interval_min);

  AddItem(_("Auto-Refresh (minutes)"), [this](){
    /* the interval as the picker edits it */
    int minutes = settings.refresh_interval_min;
    if (PickNumber(_("Auto-Refresh (minutes)"),
                   _("Automatically refresh NOTAMs every X minutes. Set to 0 to disable."),
                   0, MAX_NOTAM_REFRESH_INTERVAL_MIN, 15, minutes,
                   [](StaticString<32> &s, int v){
                     s.Format(_("%d min"), v);
                   })) {
      settings.refresh_interval_min = minutes;
      Refresh();
    }
  }, {.value = minutes.c_str(), .chevron = true});

  /* the NOTAMs which are loaded, and the button which loads them
     again */
  AddGroup();

  AddItem(_("List"), [this](){ OnListButton(); }, {.chevron = true});

  AddButton(_("Refresh"), [this](){ OnUpdateButton(); });

  AddGroup(_("Filter"));

  StaticString<64> count;

  AddToggleItem(_("Show IFR-Only NOTAMs"),
                _("Include NOTAMs for IFR traffic only."),
                settings.show_ifr,
                FormatFilterCount(count, stats.filtered_by_ifr));

  AddToggleItem(_("Show Only Currently Effective"),
                _("Filter out NOTAMs not currently in effect."),
                settings.show_only_effective,
                FormatFilterCount(count, stats.filtered_by_time));

  AddRadiusItem(_("Maximum NOTAM Radius"),
                _("Filter out NOTAMs with radius larger than this. Set to 0 to disable."),
                0, max_radius_km,
                FormatFilterCount(count, stats.filtered_by_radius));

  AddQCodesItem();
}

void
NOTAMConfigPanel::Show(const PixelRect &rc) noexcept
{
  if (net_components != nullptr && net_components->notam != nullptr) {
    try {
      net_components->notam->AddListener(*this);
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to register NOTAM config listener");
    }
  }

  ConfigListPanel::Show(rc);
}

void
NOTAMConfigPanel::Hide() noexcept
{
  if (net_components != nullptr && net_components->notam != nullptr)
    net_components->notam->RemoveListener(*this);

  notify.ClearNotification();

  ConfigListPanel::Hide();
}

void
NOTAMConfigPanel::OnUpdateButton() noexcept
{
  LogFormat("NOTAM: Manual update triggered from settings panel");
  const unsigned old_radius_km =
    CommonInterface::GetComputerSettings().airspace.notam.radius_km;
  const auto old_api_url =
    CommonInterface::GetComputerSettings().airspace.notam.api_base_url;

  /* write the values of the page first, so that the download uses
     them */
  bool dummy_changed = false;
  struct SavingForManualUpdate {
    bool &flag;

    explicit SavingForManualUpdate(bool &_flag) noexcept
      :flag(_flag) {
      flag = true;
    }

    ~SavingForManualUpdate() noexcept {
      flag = false;
    }
  } saving_guard{saving_for_manual_update};

  Save(dummy_changed);

  const auto &computer_settings = CommonInterface::GetComputerSettings();
  const bool notam_enabled =
    computer_settings.airspace.notam.enabled;
  const bool radius_changed =
    old_radius_km != computer_settings.airspace.notam.radius_km;
  const bool api_url_changed =
    old_api_url != computer_settings.airspace.notam.api_base_url;

  if (net_components == nullptr || net_components->notam == nullptr ||
      !notam_enabled)
    return;

  const auto &basic = CommonInterface::Basic();
  if (!basic.location_available || !basic.location.IsValid()) {
    UpdateFilterCounts();
    ShowMessageBox(_("No valid location."), C_("Menu", "NOTAM"),
                   MB_OK | MB_ICONEXCLAMATION);
    return;
  }

  net_components->notam->ResetFetchFailureNotification();

  if (net_components->notam->ForceUpdateLocation(basic.location,
                                                 radius_changed ||
                                                 api_url_changed)) {
    /* the counts say "Loading..." until the loader reports */
    loading = true;
    Refresh();
    net_components->notam->MarkManualRefreshRequested();
  }
}

void
NOTAMConfigPanel::OnListButton() noexcept
{
  ShowNOTAMListDialog(UIGlobals::GetMainWindow());

  /* hiding a Q-code from the list changes the shared settings while
     this page is open: take that over, so that Save() does not undo
     it */
  settings.hidden_qcodes =
    CommonInterface::GetComputerSettings().airspace.notam.hidden_qcodes;
  Refresh();
}

void
NOTAMConfigPanel::UpdateFilterCounts() noexcept
{
  if (net_components == nullptr || net_components->notam == nullptr)
    return;

  stats = net_components->notam->GetFilterStats();
  loading = false;
  Refresh();
}

void
NOTAMConfigPanel::OnNOTAMsUpdated() noexcept
{
  /* called from the loader thread: the page redraws in the UI thread */
  notify.SendNotification();
}

void
NOTAMConfigPanel::OnNOTAMsLoadComplete(
  [[maybe_unused]] NOTAMLoadNotification notification) noexcept
{
  /* whether it has succeeded or not, the counts are current now */
  notify.SendNotification();
}

bool
NOTAMConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  NOTAMSettings &live = computer.notam;
  const bool was_enabled = live.enabled;
  const unsigned old_radius_km = live.radius_km;
  const auto old_api_url = live.api_base_url;

  changed |= Profile::Update(ProfileKeys::NOTAMEnabled,
                             live.enabled, settings.enabled);

  if (live.api_base_url != settings.api_base_url) {
    live.api_base_url = settings.api_base_url;
    Profile::Set(ProfileKeys::NOTAMApiUrl, live.api_base_url.c_str());
    changed = true;
  }

  changed |= Profile::Update(ProfileKeys::NOTAMRefreshInterval,
                             live.refresh_interval_min,
                             std::min(settings.refresh_interval_min,
                                      MAX_NOTAM_REFRESH_INTERVAL_MIN));

  changed |= Profile::Update(ProfileKeys::NOTAMRadius, live.radius_km,
                             std::clamp(settings.radius_km, 1u,
                                        MAX_NOTAM_REQUEST_RADIUS_KM));

  const bool show_ifr_changed =
    Profile::Update(ProfileKeys::NOTAMShowIFR,
                    live.show_ifr, settings.show_ifr);
  const bool show_only_effective_changed =
    Profile::Update(ProfileKeys::NOTAMShowOnlyEffective,
                    live.show_only_effective, settings.show_only_effective);
  const bool filter_flags_changed =
    show_ifr_changed || show_only_effective_changed;
  changed |= filter_flags_changed;

  const bool max_radius_changed =
    Profile::Update(ProfileKeys::NOTAMMaxRadius, live.max_radius_m,
                    max_radius_km * 1000);
  changed |= max_radius_changed;

  bool qcodes_changed = false;
  if (live.hidden_qcodes != settings.hidden_qcodes) {
    live.hidden_qcodes = settings.hidden_qcodes;
    Profile::Set(ProfileKeys::NOTAMHiddenQCodes, live.hidden_qcodes.c_str());
    qcodes_changed = changed = true;
  }

  const bool radius_changed = old_radius_km != live.radius_km;
  const bool api_url_changed = old_api_url != live.api_base_url;

  if (net_components != nullptr && net_components->notam != nullptr)
    net_components->notam->SetSettings(live);

  if (was_enabled && !live.enabled) {
    if (net_components != nullptr && net_components->notam != nullptr) {
      try {
        const ScopeSuspendAllThreads suspend;
        net_components->notam->Clear();
        if (data_components != nullptr &&
            data_components->airspaces != nullptr)
          net_components->notam->UpdateAirspaces(*data_components->airspaces);
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to clear NOTAMs after disabling");
      }

      try {
        net_components->notam->InvalidateCache();
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to invalidate NOTAM cache");
      }
    }
  } else {
    const bool filters_changed =
      filter_flags_changed || max_radius_changed || qcodes_changed;
    if (net_components != nullptr && net_components->notam != nullptr &&
        data_components != nullptr && data_components->airspaces != nullptr &&
        live.enabled) {
      try {
        const bool enabled_changed = !was_enabled && live.enabled;
        if ((enabled_changed || radius_changed || api_url_changed) &&
            !saving_for_manual_update) {
          const auto &basic = CommonInterface::Basic();
          if (basic.location_available && basic.location.IsValid())
            net_components->notam->ForceUpdateLocation(basic.location, true);
        }

        if (filters_changed) {
          const ScopeSuspendAllThreads suspend;
          net_components->notam->UpdateAirspaces(*data_components->airspaces);
          if (data_components->terrain != nullptr)
            SetAirspaceGroundLevels(*data_components->airspaces,
                                    *data_components->terrain);
        }
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to apply NOTAM settings changes");
      }
    }
  }

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateNOTAMConfigPanel()
{
  return std::make_unique<NOTAMConfigPanel>();
}
