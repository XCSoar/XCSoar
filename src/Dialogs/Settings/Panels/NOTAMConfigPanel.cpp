// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMConfigPanel.hpp"
#include "LogFile.hpp"
#include "ConfigPanel.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceGlue.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "net/http/Features.hpp"
#include "NetComponents.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#include "NOTAM/Config.hpp"
#include "Protection.hpp"
#include "Dialogs/Airspace/NOTAMList.hpp"
#include "Dialogs/Message.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "ui/event/Notify.hpp"
#include "util/Macros.hpp"
#include "util/StringFormat.hpp"

#include <cmath>
#include <algorithm>
#include <cstring>
#include <exception>

enum ControlIndex {
#ifdef HAVE_HTTP
  ENABLE_NOTAM,
  API_URL,
  NOTAM_RADIUS,
  REFRESH_INTERVAL,
  FILTER_SPACER,
  SHOW_IFR,
  IFR_FILTERED,
  SHOW_ONLY_EFFECTIVE,
  TIME_FILTERED,
  MAX_RADIUS,
  RADIUS_FILTERED,
  HIDDEN_QCODES,
  QCODE_FILTERED,
#endif
};

class NOTAMConfigPanel : public RowFormWidget, 
                         public DataFieldListener,
                         public NOTAMListener {
  // UI thread notification for async updates
  UI::Notify notify{[this]() { UpdateFilterCounts(); }};
  bool saving_for_manual_update = false;

public:
  NOTAMConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnUpdateButton() noexcept;
  void SetFilterRowCount(unsigned control, unsigned count) noexcept;
  void SetFilterRowLoading(unsigned control) noexcept;
  void UpdateVisibility() noexcept;
  void UpdateFilterCounts() noexcept;
  void ShowLoadingStatus() noexcept;
  
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
  
  /* methods from NOTAMListener */
  void OnNOTAMsUpdated() noexcept override;
  void OnNOTAMsLoadComplete(bool success) noexcept override;
};

void
NOTAMConfigPanel::Prepare([[maybe_unused]] ContainerWindow &parent, 
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
#ifdef HAVE_HTTP
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;

  AddBoolean(_("NOTAM Support"),
             _("Enable downloading and display of NOTAMs from aviation authorities."),
             computer.notam.enabled, this);

  AddText(_("API URL"),
          _("Base URL of the NOTAM proxy API. Must be configured before NOTAMs can be fetched."),
          computer.notam.api_base_url.c_str());

  Unit distance_unit = Units::GetUserDistanceUnit();
  double radius_user = Units::ToUserDistance(computer.notam.radius_km * 1000.0);
  const char *unit_name = Units::GetUnitName(distance_unit);

  char radius_format_display[32];
  const char radius_format_edit[] = "%.0f";
  StringFormat(radius_format_display,
               ARRAY_SIZE(radius_format_display),
               _("%%.0f %s"), unit_name);

  const double min_search_radius_user = Units::ToUserDistance(1000.0);
  const double max_search_radius_user =
    Units::ToUserDistance(MAX_NOTAM_REQUEST_RADIUS_KM * 1000.0);
  const double step_search_radius_user = Units::ToUserDistance(10000.0);

  AddFloat(_("Search Radius"),
           _("Radius around current location to fetch NOTAMs."),
           radius_format_display, radius_format_edit,
           min_search_radius_user, max_search_radius_user, step_search_radius_user, 0,
           radius_user);

  AddInteger(_("Auto-Refresh (minutes)"),
             _("Automatically refresh NOTAMs every X minutes. Set to 0 to disable."),
             _("%d min"), "%d", 0, MAX_NOTAM_REFRESH_INTERVAL_MIN, 15,
             computer.notam.refresh_interval_min);

  // Get NOTAM statistics for filter counts
  NOTAMGlue::FilterStats stats = {};
  if (net_components && net_components->notam) {
    stats = net_components->notam->GetFilterStats();
  }

  AddSpacer();
  
  char buffer[64];

  AddBoolean(_("Show IFR-Only NOTAMs"),
             _("Include NOTAMs for IFR traffic only."),
             computer.notam.show_ifr);
  StringFormat(buffer, ARRAY_SIZE(buffer),
               _("%u filtered"), stats.filtered_by_ifr);
  AddReadOnly("", nullptr, buffer);

  AddBoolean(_("Show Only Currently Effective"),
             _("Filter out NOTAMs not currently in effect."),
             computer.notam.show_only_effective);
  StringFormat(buffer, ARRAY_SIZE(buffer),
               _("%u filtered"), stats.filtered_by_time);
  AddReadOnly("", nullptr, buffer);

  // Radius filter with user units
  double max_radius_user = Units::ToUserDistance(computer.notam.max_radius_m);
  const double min_max_radius_user = Units::ToUserDistance(0.0);
  const double max_max_radius_user = max_search_radius_user;
  const double step_max_radius_user = step_search_radius_user;
  
  char format_display[32];
  const char format_edit[] = "%.0f";
  StringFormat(format_display, ARRAY_SIZE(format_display),
               _("%%.0f %s"), unit_name);
  
  AddFloat(_("Maximum NOTAM Radius"),
           _("Filter out NOTAMs with radius larger than this. Set to 0 to disable."),
           format_display, format_edit,
           min_max_radius_user, max_max_radius_user, step_max_radius_user, 0,
           max_radius_user);
  StringFormat(buffer, ARRAY_SIZE(buffer),
               _("%u filtered"), stats.filtered_by_radius);
  AddReadOnly("", nullptr, buffer);

  AddText(_("Hidden Q-Codes"),
          _("Space-separated Q-code prefixes to hide (e.g., QA QK QN QOA QOL)."),
          computer.notam.hidden_qcodes.c_str());
  StringFormat(buffer, ARRAY_SIZE(buffer),
               _("%u filtered"), stats.filtered_by_qcode);
  AddReadOnly("", nullptr, buffer);

  UpdateVisibility();
#endif
}

void
NOTAMConfigPanel::Show(const PixelRect &rc) noexcept
{
#ifdef HAVE_HTTP
  // Register as listener for NOTAM updates
  if (net_components && net_components->notam) {
    try {
      net_components->notam->AddListener(*this);
    } catch (const std::exception &e) {
      LogFmt("Failed to register NOTAM config listener: {}", e.what());
    } catch (...) {
      LogError(std::current_exception(),
               "Failed to register NOTAM config listener");
    }
  }
  
  ConfigPanel::BorrowExtraButton(1, _("Refresh"), [this](){
    OnUpdateButton();
  });
  
  ConfigPanel::BorrowExtraButton(2, _("List"), [](){
    ShowNOTAMListDialog(UIGlobals::GetMainWindow());
  });
#endif

  RowFormWidget::Show(rc);
}

void
NOTAMConfigPanel::Hide() noexcept
{
#ifdef HAVE_HTTP
  // Unregister as listener
  if (net_components && net_components->notam) {
    net_components->notam->RemoveListener(*this);
  }

  notify.ClearNotification();
  
  ConfigPanel::ReturnExtraButton(1);
  ConfigPanel::ReturnExtraButton(2);
#endif

  RowFormWidget::Hide();
}

void
NOTAMConfigPanel::OnUpdateButton() noexcept
{
#ifdef HAVE_HTTP
  LogFormat("NOTAM: Manual update triggered from settings panel");
  const unsigned old_radius_km =
    CommonInterface::GetComputerSettings().airspace.notam.radius_km;
  const auto old_api_url =
    CommonInterface::GetComputerSettings().airspace.notam.api_base_url;

  // Save current settings first so the update uses the new values
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

  if (net_components && net_components->notam && notam_enabled) {
    const auto &basic = CommonInterface::Basic();
    if (!basic.location_available || !basic.location.IsValid()) {
      UpdateFilterCounts();
      ShowMessageBox(_("No valid location."), _("NOTAM"),
                     MB_OK | MB_ICONEXCLAMATION);
      return;
    }

    net_components->notam->ResetFetchFailureNotification();

    if (net_components->notam->ForceUpdateLocation(basic.location,
                                                   radius_changed ||
                                                   api_url_changed)) {
      ShowLoadingStatus();
      net_components->notam->MarkManualRefreshRequested();
    }
  }
#endif
}

void
NOTAMConfigPanel::SetFilterRowCount(const unsigned control,
                                    const unsigned count) noexcept
{
#ifdef HAVE_HTTP
  char buffer[64];
  StringFormat(buffer, ARRAY_SIZE(buffer), _("%u filtered"), count);
  SetText(control, buffer);
#else
  (void)control;
  (void)count;
#endif
}

void
NOTAMConfigPanel::SetFilterRowLoading(const unsigned control) noexcept
{
#ifdef HAVE_HTTP
  SetText(control, _("Loading..."));
#else
  (void)control;
#endif
}

void
NOTAMConfigPanel::UpdateFilterCounts() noexcept
{
#ifdef HAVE_HTTP
  if (!net_components || !net_components->notam) {
    return;
  }

  // Get current filter statistics
  NOTAMGlue::FilterStats stats = net_components->notam->GetFilterStats();
  
  SetFilterRowCount(IFR_FILTERED, stats.filtered_by_ifr);
  SetFilterRowCount(TIME_FILTERED, stats.filtered_by_time);
  SetFilterRowCount(RADIUS_FILTERED, stats.filtered_by_radius);
  SetFilterRowCount(QCODE_FILTERED, stats.filtered_by_qcode);
#endif
}

void
NOTAMConfigPanel::ShowLoadingStatus() noexcept
{
#ifdef HAVE_HTTP
  SetFilterRowLoading(IFR_FILTERED);
  SetFilterRowLoading(TIME_FILTERED);
  SetFilterRowLoading(RADIUS_FILTERED);
  SetFilterRowLoading(QCODE_FILTERED);
#endif
}

void
NOTAMConfigPanel::OnNOTAMsUpdated() noexcept
{
#ifdef HAVE_HTTP
  // Called from background thread - dispatch UI update to main thread
  notify.SendNotification();
#endif
}

void
NOTAMConfigPanel::OnNOTAMsLoadComplete([[maybe_unused]] bool success) noexcept
{
#ifdef HAVE_HTTP
  // Always refresh panel state when a load attempt finishes.
  notify.SendNotification();
#endif
}

void
NOTAMConfigPanel::UpdateVisibility() noexcept
{
#ifdef HAVE_HTTP
  const DataFieldBoolean &df =
    static_cast<const DataFieldBoolean &>(GetDataField(ENABLE_NOTAM));
  const bool enabled = df.GetValue();
  
  SetRowAvailable(NOTICE, enabled);
  SetRowAvailable(API_URL, enabled);
  SetRowAvailable(NOTAM_RADIUS, enabled);
  SetRowAvailable(REFRESH_INTERVAL, enabled);
  SetRowAvailable(FILTER_SPACER, enabled);
  SetRowAvailable(SHOW_IFR, enabled);
  SetRowAvailable(IFR_FILTERED, enabled);
  SetRowAvailable(SHOW_ONLY_EFFECTIVE, enabled);
  SetRowAvailable(TIME_FILTERED, enabled);
  SetRowAvailable(MAX_RADIUS, enabled);
  SetRowAvailable(RADIUS_FILTERED, enabled);
  SetRowAvailable(HIDDEN_QCODES, enabled);
  SetRowAvailable(QCODE_FILTERED, enabled);
#endif
}

void
NOTAMConfigPanel::OnModified(DataField &df) noexcept
{
#ifdef HAVE_HTTP
  if (IsDataField(ENABLE_NOTAM, df)) {
    UpdateVisibility();
  }
#endif
}

bool
NOTAMConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

#ifdef HAVE_HTTP
  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  const bool was_enabled = computer.notam.enabled;
  const unsigned old_radius_km = computer.notam.radius_km;
  const auto old_api_url = computer.notam.api_base_url;

  changed |= SaveValue(ENABLE_NOTAM, ProfileKeys::NOTAMEnabled, computer.notam.enabled);
  changed |= SaveValue(API_URL, ProfileKeys::NOTAMApiUrl, computer.notam.api_base_url);
  changed |= SaveValueInteger(REFRESH_INTERVAL,
                              ProfileKeys::NOTAMRefreshInterval,
                              computer.notam.refresh_interval_min);
  const unsigned clamped_refresh_interval =
    std::clamp(computer.notam.refresh_interval_min, 0u,
               MAX_NOTAM_REFRESH_INTERVAL_MIN);
  if (computer.notam.refresh_interval_min != clamped_refresh_interval) {
    computer.notam.refresh_interval_min = clamped_refresh_interval;
    Profile::Set(ProfileKeys::NOTAMRefreshInterval, clamped_refresh_interval);
    changed = true;
  }

  // Search radius - convert from user units to kilometers
  double radius_user = GetValueFloat(NOTAM_RADIUS);
  unsigned radius_km = static_cast<unsigned>(
    std::lround(Units::ToSysDistance(radius_user) / 1000.0));
  if (radius_km < 1)
    radius_km = 1;
  if (radius_km > MAX_NOTAM_REQUEST_RADIUS_KM)
    radius_km = MAX_NOTAM_REQUEST_RADIUS_KM;
  if (computer.notam.radius_km != radius_km) {
    computer.notam.radius_km = radius_km;
    Profile::Set(ProfileKeys::NOTAMRadius, radius_km);
    changed = true;
  }

  // Filter settings
  const bool show_ifr_changed =
    SaveValue(SHOW_IFR, ProfileKeys::NOTAMShowIFR, computer.notam.show_ifr);
  const bool show_only_effective_changed =
    SaveValue(SHOW_ONLY_EFFECTIVE, ProfileKeys::NOTAMShowOnlyEffective,
              computer.notam.show_only_effective);
  const bool filter_flags_changed =
    show_ifr_changed || show_only_effective_changed;
  changed |= filter_flags_changed;
  
  // Radius filter - convert from user units to meters
  double max_radius_user = GetValueFloat(MAX_RADIUS);
  unsigned max_radius_m =
    static_cast<unsigned>(std::lround(Units::ToSysDistance(max_radius_user)));
  bool max_radius_changed = false;
  if (computer.notam.max_radius_m != max_radius_m) {
    computer.notam.max_radius_m = max_radius_m;
    Profile::Set(ProfileKeys::NOTAMMaxRadius, max_radius_m);
    changed = true;
    max_radius_changed = true;
  }
  
  const bool qcodes_changed =
    SaveValue(HIDDEN_QCODES, ProfileKeys::NOTAMHiddenQCodes,
              computer.notam.hidden_qcodes);
  changed |= qcodes_changed;

  const bool radius_changed = old_radius_km != computer.notam.radius_km;
  const bool api_url_changed = old_api_url != computer.notam.api_base_url;

  if (net_components != nullptr && net_components->notam != nullptr)
    net_components->notam->SetSettings(computer.notam);

  if (was_enabled && !computer.notam.enabled) {
    if (net_components != nullptr && net_components->notam != nullptr) {
      try {
        const ScopeSuspendAllThreads suspend;
        net_components->notam->Clear();
        net_components->notam->InvalidateCache();
        if (data_components != nullptr && data_components->airspaces != nullptr)
          net_components->notam->UpdateAirspaces(*data_components->airspaces);
      } catch (const std::exception &e) {
        LogFmt("Failed to clear NOTAMs after disabling: {}", e.what());
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to clear NOTAMs after disabling");
      }
    }
  } else {
    const bool filters_changed =
      filter_flags_changed || max_radius_changed || qcodes_changed;
    if (net_components != nullptr && net_components->notam != nullptr &&
        data_components != nullptr && data_components->airspaces != nullptr &&
        computer.notam.enabled) {
      try {
        if ((radius_changed || api_url_changed) && !saving_for_manual_update) {
          const auto &basic = CommonInterface::Basic();
          if (basic.location_available && basic.location.IsValid()) {
            net_components->notam->ForceUpdateLocation(basic.location, true);
          }
        }

        if (filters_changed) {
          const ScopeSuspendAllThreads suspend;
          net_components->notam->UpdateAirspaces(*data_components->airspaces);
          if (data_components->terrain != nullptr)
            SetAirspaceGroundLevels(*data_components->airspaces,
                                    *data_components->terrain);
        }
      } catch (const std::exception &e) {
        LogFmt("Failed to apply NOTAM settings changes: {}", e.what());
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to apply NOTAM settings changes");
      }
    }
  }
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateNOTAMConfigPanel()
{
  return std::make_unique<NOTAMConfigPanel>();
}
