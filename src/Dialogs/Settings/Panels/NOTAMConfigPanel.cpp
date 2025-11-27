
// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LogFile.hpp"
#include "NOTAMConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"
#include "Form/DataField/Boolean.hpp"
#include "Profile/Keys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "net/http/Features.hpp"
#include "NetComponents.hpp"
#include "Components.hpp"
#include "NOTAM/Glue.hpp"
#include "Formatter/UserUnits.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <ctime>

enum ControlIndex {
#ifdef HAVE_HTTP
  EnableNOTAM,
  NOTAMRadius,
  RefreshInterval,
  LastUpdate,
  DistanceFromLastUpdate,
  FeatureTypeFilterSpacer,
  ShowAirspace,
  ShowObst,
  ShowMilitary,
  ShowOther
#endif
};

class NOTAMConfigPanel : public RowFormWidget, DataFieldListener {
  UI::PeriodicTimer timer{[this]{ OnTimer(); }};

public:
  NOTAMConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnUpdateButton() noexcept;
  void UpdateVisibility() noexcept;
  void RefreshDisplayFields() noexcept;
  void OnTimer() noexcept;
  
  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

void
NOTAMConfigPanel::Prepare([[maybe_unused]] ContainerWindow &parent, 
                          [[maybe_unused]] const PixelRect &rc) noexcept
{
#ifdef HAVE_HTTP
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;

  AddBoolean(_("NOTAM Support"),
             _("Enable downloading and display of NOTAMs (Notice to Airmen) from aviation authorities. "
               "NOTAMs provide temporary airspace restrictions and operational information."),
             computer.notam.enabled, this);

  // Add all rows but control visibility based on enabled state
    AddInteger(_("NOTAM Radius (km)"),
               _("Search radius around current location for NOTAMs in kilometers. "
                 "Larger values download more NOTAMs but may affect performance."),
               _T("%d km"), _T("%d"), 1, 500, 10,
               computer.notam.radius_km);

    AddInteger(_("Auto-Refresh Interval"),
               _("Automatically refresh NOTAMs every X minutes during flight. Set to 0 to disable automatic updates."),
               _T("%d min"), _T("%d"), 0, 240, 15,
               computer.notam.refresh_interval_min);

    // Display last update time & distance from update location
    std::time_t last_update = 0;
    GeoPoint last_loc = GeoPoint::Invalid();
    if (net_components && net_components->notam) {
      last_update = net_components->notam->GetLastUpdateTime();
      last_loc = net_components->notam->GetLastUpdateLocation();
    }

    if (last_update > 0) {
      char time_buffer[32];
      std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M", std::localtime(&last_update));
      AddReadOnly(_("Last Update"), nullptr, time_buffer);
    } else {
      AddReadOnly(_("Last Update"), nullptr, _("Never"));
    }

    // Distance from last update location (if both valid)
    const auto &basic = CommonInterface::Basic();
    if (basic.location.IsValid() && last_loc.IsValid()) {
      double dist_m = basic.location.Distance(last_loc);
      TCHAR dist_buffer[32];
      // Use smart formatting (switching to small units when appropriate)
      FormatUserDistanceSmart(dist_m, dist_buffer, true, 1000.0, 9.999);
      AddReadOnly(_("Distance From Last Update"), nullptr, dist_buffer);
    } else {
      AddReadOnly(_("Distance From Last Update"), nullptr, _("Unknown"));
    }

    // Feature type filtering (simplified for glider pilots)
    AddSpacer();
    AddBoolean(_("Show AIRSPACE NOTAMs"),
               _("Airspace restrictions and changes (essential for flight planning)."),
               computer.notam.show_airspace);

    AddBoolean(_("Show OBST (Obstacle) NOTAMs"),
               _("Obstacles, towers, cranes, construction (important for low-level flying)."),
               computer.notam.show_obst);

    AddBoolean(_("Show MILITARY NOTAMs"),
               _("Military exercises and operations (can create temporary restricted airspace)."),
               computer.notam.show_military);

    AddBoolean(_("Show Other NOTAMs"),
               _("All other NOTAM types including airports, navigation aids, procedures, etc."),
               computer.notam.show_other);
    SetExpertRow(ShowOther);

  // Set initial visibility based on enabled state
  UpdateVisibility();
#endif
}

void
NOTAMConfigPanel::Show(const PixelRect &rc) noexcept
{
#ifdef HAVE_HTTP
  ConfigPanel::BorrowExtraButton(1, _("Update Now"), [this](){
    OnUpdateButton();
  });
  
  // Start periodic timer to refresh display fields (every 2 seconds)
  timer.Schedule(std::chrono::seconds(2));
#endif

  RowFormWidget::Show(rc);
}

void
NOTAMConfigPanel::OnUpdateButton() noexcept
{
  LogFormat("NOTAM: Manual update triggered from settings panel");
#ifdef HAVE_HTTP
  if (net_components && net_components->notam) {
    // Invalidate cache to force fresh fetch
    net_components->notam->InvalidateCache();
    
    const auto &basic = CommonInterface::Basic();
    if (basic.location.IsValid()) {
      // Trigger NOTAM update for current location
      net_components->notam->UpdateLocation(basic.location);
      // Display fields will be refreshed by the periodic timer
    }
  }
#endif
}

void
NOTAMConfigPanel::UpdateVisibility() noexcept
{
#ifdef HAVE_HTTP
  const DataFieldBoolean &df = (const DataFieldBoolean &)GetDataField(EnableNOTAM);
  const bool enabled = df.GetValue();
  
  SetRowAvailable(NOTAMRadius, enabled);
  SetRowAvailable(RefreshInterval, enabled);
  SetRowAvailable(LastUpdate, enabled);
  SetRowAvailable(DistanceFromLastUpdate, enabled);
  SetRowAvailable(FeatureTypeFilterSpacer, enabled);
  SetRowAvailable(ShowAirspace, enabled);
  SetRowAvailable(ShowObst, enabled);
  SetRowAvailable(ShowMilitary, enabled);
  SetRowAvailable(ShowOther, enabled);
#endif
}

void
NOTAMConfigPanel::RefreshDisplayFields() noexcept
{
#ifdef HAVE_HTTP
  if (!net_components || !net_components->notam)
    return;

  // Refresh last update time
  std::time_t last_update = net_components->notam->GetLastUpdateTime();
  if (last_update > 0) {
    char time_buffer[32];
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M", std::localtime(&last_update));
    SetText(LastUpdate, time_buffer);
  } else {
    SetText(LastUpdate, _("Never"));
  }

  // Refresh distance from last update location
  const auto &basic = CommonInterface::Basic();
  GeoPoint last_loc = net_components->notam->GetLastUpdateLocation();
  
  // Debug logging
  LogFormat("NOTAM Panel: current_loc=%.6f,%.6f (available=%d, valid=%d), last_loc=%.6f,%.6f (valid=%d)",
            basic.location.latitude.Degrees(), basic.location.longitude.Degrees(),
            (int)basic.location_available.IsValid(), (int)basic.location.IsValid(),
            last_loc.latitude.Degrees(), last_loc.longitude.Degrees(),
            (int)last_loc.IsValid());
  
  // Check both that location is valid AND that it's actually available (has GPS fix)
  if (basic.location_available && basic.location.IsValid() && last_loc.IsValid()) {
    double dist_m = basic.location.Distance(last_loc);
    LogFormat("NOTAM Panel: distance = %.2f m (%.2f km)", dist_m, dist_m / 1000.0);
    TCHAR dist_buffer[32];
    FormatUserDistanceSmart(dist_m, dist_buffer, true, 1000.0, 9.999);
    SetText(DistanceFromLastUpdate, dist_buffer);
  } else {
    SetText(DistanceFromLastUpdate, _("Unknown"));
  }
#endif
}

void
NOTAMConfigPanel::OnTimer() noexcept
{
  RefreshDisplayFields();
}

void
NOTAMConfigPanel::OnModified(DataField &df) noexcept
{
#ifdef HAVE_HTTP
  if (IsDataField(EnableNOTAM, df)) {
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

  changed |= SaveValue(EnableNOTAM, ProfileKeys::NOTAMEnabled, computer.notam.enabled);
  changed |= SaveValueInteger(NOTAMRadius, ProfileKeys::NOTAMRadius, computer.notam.radius_km);
  changed |= SaveValueInteger(RefreshInterval, ProfileKeys::NOTAMRefreshInterval, computer.notam.refresh_interval_min);

  // Feature type filtering (simplified)
  changed |= SaveValue(ShowAirspace, ProfileKeys::NOTAMShowAirspace, computer.notam.show_airspace);
  changed |= SaveValue(ShowObst, ProfileKeys::NOTAMShowObst, computer.notam.show_obst);
  changed |= SaveValue(ShowMilitary, ProfileKeys::NOTAMShowMilitary, computer.notam.show_military);
  changed |= SaveValue(ShowOther, ProfileKeys::NOTAMShowOther, computer.notam.show_other);
#endif

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateNOTAMConfigPanel()
{
  return std::make_unique<NOTAMConfigPanel>();
}
