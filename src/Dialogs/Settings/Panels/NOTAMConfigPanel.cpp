
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogFile.hpp"
#include "NOTAMConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "net/http/Features.hpp"
#include "NetComponents.hpp"
#include "Components.hpp"
#include "DataComponents.hpp"
#include "NOTAM/NOTAMGlue.hpp"
#include "Dialogs/Airspace/NOTAMList.hpp"
#include "Formatter/UserUnits.hpp"
#include "Units/Units.hpp"
#include "Units/Descriptor.hpp"
#include "ui/event/Notify.hpp"
#include "util/StringFormat.hpp"

enum ControlIndex {
#ifdef HAVE_HTTP
  ENABLE_NOTAM,
  NOTAM_RADIUS,
  REFRESH_INTERVAL,
  MAX_NOTAMS,
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
                         DataFieldListener,
                         NOTAMListener {
  // UI thread notification for async updates
  UI::Notify notify{[this]() { UpdateFilterCounts(); }};

public:
  NOTAMConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnUpdateButton() noexcept;
  void UpdateVisibility() noexcept;
  void UpdateFilterCounts() noexcept;
  void ShowLoadingStatus() noexcept;
  
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
  
  /* methods from NOTAMListener */
  void OnNOTAMsUpdated() noexcept override;
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

  Unit distance_unit = Units::GetUserDistanceUnit();
  double radius_user = Units::ToUserDistance(computer.notam.radius_km * 1000.0);
  const TCHAR *unit_name = Units::GetUnitName(distance_unit);

  TCHAR radius_format_display[32], radius_format_edit[32];
  StringFormat(radius_format_display,
               sizeof(radius_format_display) / sizeof(radius_format_display[0]),
               _T("%%.0f %s"), unit_name);
  _tcscpy(radius_format_edit, _T("%.0f"));

  const double min_search_radius_user = Units::ToUserDistance(1000.0);
  const double max_search_radius_user = Units::ToUserDistance(500000.0);
  const double step_search_radius_user = Units::ToUserDistance(10000.0);

  AddFloat(_("Search Radius"),
           _("Radius around current location to fetch NOTAMs."),
           radius_format_display, radius_format_edit,
           min_search_radius_user, max_search_radius_user, step_search_radius_user, 0,
           radius_user);

  AddInteger(_("Auto-Refresh (minutes)"),
             _("Automatically refresh NOTAMs every X minutes. Set to 0 to disable."),
             _T("%d min"), _T("%d"), 0, 240, 15,
             computer.notam.refresh_interval_min);

  AddInteger(_("Maximum NOTAM Count"),
             _("Limit the total number of NOTAMs to fetch."),
             _T("%u"), _T("%u"), 1, 1000, 100,
             computer.notam.max_notams);
  SetExpertRow(MAX_NOTAMS);

  // Get NOTAM statistics for filter counts
  NOTAMGlue::FilterStats stats = {};
  if (net_components && net_components->notam) {
    stats = net_components->notam->GetFilterStats();
  }

  AddSpacer();
  
  TCHAR buffer[64];

  AddBoolean(_("Show IFR-Only NOTAMs"),
             _("Include NOTAMs for IFR traffic only."),
             computer.notam.show_ifr);
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_ifr);
  AddReadOnly(_T(""), nullptr, buffer);

  AddBoolean(_("Show Only Currently Effective"),
             _("Filter out NOTAMs not currently in effect."),
             computer.notam.show_only_effective);
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_time);
  AddReadOnly(_T(""), nullptr, buffer);

  // Radius filter with user units
  double max_radius_user = Units::ToUserDistance(computer.notam.max_radius_m);
  
  TCHAR format_display[32], format_edit[32];
  StringFormat(format_display, sizeof(format_display) / sizeof(format_display[0]),
               _T("%%.0f %s"), unit_name);
  _tcscpy(format_edit, _T("%.0f"));
  
  AddFloat(_("Maximum NOTAM Radius"),
           _("Filter out NOTAMs with radius larger than this. Set to 0 to disable."),
           format_display, format_edit,
           0, 1000, 10, 0,
           max_radius_user);
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_radius);
  AddReadOnly(_T(""), nullptr, buffer);

  AddText(_("Hidden Q-Codes"),
          _("Space-separated Q-code prefixes to hide (e.g., QA QK QN QOA QOL)."),
          computer.notam.hidden_qcodes.c_str());
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_qcode);
  AddReadOnly(_T(""), nullptr, buffer);

  UpdateVisibility();
#endif
}

void
NOTAMConfigPanel::Show(const PixelRect &rc) noexcept
{
#ifdef HAVE_HTTP
  // Register as listener for NOTAM updates
  if (net_components && net_components->notam) {
    net_components->notam->AddListener(*this);
  }
  
  ConfigPanel::BorrowExtraButton(1, _("Refresh"), [this](){
    OnUpdateButton();
  });
  
  ConfigPanel::BorrowExtraButton(2, _("List"), [](){
    LogFormat("NOTAM: View NOTAMs button clicked");
    ShowNOTAMListDialog(UIGlobals::GetMainWindow());
    LogFormat("NOTAM: ShowNOTAMListDialog returned");
  });
#endif

  RowFormWidget::Show(rc);
}

void
NOTAMConfigPanel::Hide() noexcept
{
  RowFormWidget::Hide();
#ifdef HAVE_HTTP
  // Unregister as listener
  if (net_components && net_components->notam) {
    net_components->notam->RemoveListener(*this);
  }
  
  ConfigPanel::ReturnExtraButton(1);
  ConfigPanel::ReturnExtraButton(2);
#endif
}

void
NOTAMConfigPanel::OnUpdateButton() noexcept
{
  LogFormat("NOTAM: Manual update triggered from settings panel");
#ifdef HAVE_HTTP
  // Save current settings first so the update uses the new values
  bool dummy_changed = false;
  Save(dummy_changed);

  if (net_components && net_components->notam) {
    // Show loading status
    ShowLoadingStatus();
    
    // Invalidate cache to force fresh fetch with new settings
    net_components->notam->InvalidateCache();
    
    const auto &basic = CommonInterface::Basic();
    if (basic.location.IsValid()) {
      // Trigger NOTAM update for current location (async)
      // Filter counts will update via OnNOTAMsUpdated() callback when complete
      net_components->notam->UpdateLocation(basic.location);
    }
  }
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
  
  TCHAR buffer[64];
  
  // Update each read-only count display
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_ifr);
  SetText(IFR_FILTERED, buffer);
  
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_time);
  SetText(TIME_FILTERED, buffer);
  
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_radius);
  SetText(RADIUS_FILTERED, buffer);
  
  StringFormat(buffer, sizeof(buffer) / sizeof(buffer[0]),
               _("%u filtered"), stats.filtered_by_qcode);
  SetText(QCODE_FILTERED, buffer);
#endif
}

void
NOTAMConfigPanel::ShowLoadingStatus() noexcept
{
#ifdef HAVE_HTTP
  // Show "Loading..." in all filter count displays
  SetText(IFR_FILTERED, _("Loading..."));
  SetText(TIME_FILTERED, _("Loading..."));
  SetText(RADIUS_FILTERED, _("Loading..."));
  SetText(QCODE_FILTERED, _("Loading..."));
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
NOTAMConfigPanel::UpdateVisibility() noexcept
{
#ifdef HAVE_HTTP
  const DataFieldBoolean &df =
    static_cast<const DataFieldBoolean &>(GetDataField(ENABLE_NOTAM));
  const bool enabled = df.GetValue();
  
  SetRowAvailable(NOTAM_RADIUS, enabled);
  SetRowAvailable(REFRESH_INTERVAL, enabled);
  SetRowAvailable(MAX_NOTAMS, enabled);
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

  changed |= SaveValue(ENABLE_NOTAM, ProfileKeys::NOTAMEnabled, computer.notam.enabled);
  changed |= SaveValueInteger(REFRESH_INTERVAL, ProfileKeys::NOTAMRefreshInterval, computer.notam.refresh_interval_min);
  changed |= SaveValueInteger(MAX_NOTAMS, ProfileKeys::NOTAMMaxCount, computer.notam.max_notams);

  // Search radius - convert from user units to kilometers
  double radius_user = GetValueFloat(NOTAM_RADIUS);
  unsigned radius_km = static_cast<unsigned>(
    (Units::ToSysDistance(radius_user) / 1000.0) + 0.5);
  if (radius_km < 1)
    radius_km = 1;
  if (radius_km > 500)
    radius_km = 500;
  if (computer.notam.radius_km != radius_km) {
    computer.notam.radius_km = radius_km;
    Profile::Set(ProfileKeys::NOTAMRadius, radius_km);
    changed = true;
  }

  // Filter settings
  changed |= SaveValue(SHOW_IFR, ProfileKeys::NOTAMShowIFR, computer.notam.show_ifr);
  changed |= SaveValue(SHOW_ONLY_EFFECTIVE, ProfileKeys::NOTAMShowOnlyEffective, computer.notam.show_only_effective);
  
  // Radius filter - convert from user units to meters
  double max_radius_user = GetValueFloat(MAX_RADIUS);
  unsigned max_radius_m =
    static_cast<unsigned>(Units::ToSysDistance(max_radius_user));
  if (computer.notam.max_radius_m != max_radius_m) {
    computer.notam.max_radius_m = max_radius_m;
    Profile::Set(ProfileKeys::NOTAMMaxRadius, max_radius_m);
    changed = true;
  }
  
  changed |= SaveValue(HIDDEN_QCODES, ProfileKeys::NOTAMHiddenQCodes, computer.notam.hidden_qcodes);

  if (was_enabled && !computer.notam.enabled) {
    if (net_components != nullptr && net_components->notam != nullptr) {
      net_components->notam->Clear();
      net_components->notam->InvalidateCache();
      if (data_components != nullptr && data_components->airspaces != nullptr)
        net_components->notam->UpdateAirspaces(*data_components->airspaces);
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
