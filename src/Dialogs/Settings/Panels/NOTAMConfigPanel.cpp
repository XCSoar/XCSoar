// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "Widget/RowFormWidget.hpp"
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
#include <ctime>

enum ControlIndex {
#ifdef HAVE_HTTP
  EnableNOTAM,
  NOTAMRadius,
  RefreshInterval,
  LastUpdate,
  FilterDaylightOnly,
  FilterNightOnly,
  HoursBeforeSunrise,
  HoursAfterSunset,
  FilterSeries,
  ShowAirspace,
  ShowAirport,
  ShowNavaid,
  ShowObstacle,
  ShowMilitary,
  ShowOther
#endif
};

class NOTAMConfigPanel final : public RowFormWidget {
public:
  NOTAMConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void OnUpdateButton() noexcept;
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
             computer.notam.enabled);

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

  // Time-based filtering
  AddSpacer();
  AddBoolean(_("Daylight Only"),
             _("Show only NOTAMs that are active during daylight hours (sunrise to sunset)."),
             computer.notam.filter_daylight_only);

  AddBoolean(_("Night Only"),
             _("Show only NOTAMs that are active during night hours (sunset to sunrise)."),
             computer.notam.filter_night_only);
  SetExpertRow(FilterNightOnly);

  AddInteger(_("Hours Before Sunrise"),
             _("Include NOTAMs this many hours before sunrise (-1 to disable filtering)."),
             _T("%d h"), _T("%d"), -1, 12, 1,
             computer.notam.hours_before_sunrise);
  SetExpertRow(HoursBeforeSunrise);

  AddInteger(_("Hours After Sunset"),
             _("Include NOTAMs this many hours after sunset (-1 to disable filtering)."),
             _T("%d h"), _T("%d"), -1, 12, 1,
             computer.notam.hours_after_sunset);
  SetExpertRow(HoursAfterSunset);

  // Type-based filtering
  AddSpacer();
  AddBoolean(_("Show Airspace NOTAMs"),
             _("Airspace restrictions and changes (recommended for glider pilots)."),
             computer.notam.show_airspace);

  AddBoolean(_("Show Military NOTAMs"),
             _("Military exercises creating temporary airspace (important for cross-country)."),
             computer.notam.show_military);

  AddBoolean(_("Show Obstacle NOTAMs"),
             _("New obstacles, towers, and construction (important for low-level flying)."),
             computer.notam.show_obstacle);

  AddBoolean(_("Show Airport NOTAMs"),
             _("Airport and runway information (usually not relevant for cross-country gliding)."),
             computer.notam.show_airport);
  SetExpertRow(ShowAirport);

  AddBoolean(_("Show Navigation Aid NOTAMs"),
             _("VOR, NDB, ILS status (not critical for GPS navigation)."),
             computer.notam.show_navaid);
  SetExpertRow(ShowNavaid);

  AddBoolean(_("Show Other NOTAMs"),
             _("Miscellaneous NOTAMs not covered by other categories."),
             computer.notam.show_other);
  SetExpertRow(ShowOther);
#endif
}

void
NOTAMConfigPanel::Show(const PixelRect &rc) noexcept
{
#ifdef HAVE_HTTP
  ConfigPanel::BorrowExtraButton(1, _("Update Now"), [this](){
    OnUpdateButton();
  });
#endif

  RowFormWidget::Show(rc);
}

void
NOTAMConfigPanel::OnUpdateButton() noexcept
{
  // TODO: Trigger NOTAM refresh via NetComponents
  // This will be connected to the actual NOTAM update mechanism
  // For now, just a placeholder for the button action
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

  // Time-based filtering
  changed |= SaveValue(FilterDaylightOnly, ProfileKeys::NOTAMFilterDaylightOnly, computer.notam.filter_daylight_only);
  changed |= SaveValue(FilterNightOnly, ProfileKeys::NOTAMFilterNightOnly, computer.notam.filter_night_only);
  changed |= SaveValueInteger(HoursBeforeSunrise, ProfileKeys::NOTAMHoursBeforeSunrise, computer.notam.hours_before_sunrise);
  changed |= SaveValueInteger(HoursAfterSunset, ProfileKeys::NOTAMHoursAfterSunset, computer.notam.hours_after_sunset);

  // Type-based filtering
  changed |= SaveValue(ShowAirspace, ProfileKeys::NOTAMShowAirspace, computer.notam.show_airspace);
  changed |= SaveValue(ShowAirport, ProfileKeys::NOTAMShowAirport, computer.notam.show_airport);
  changed |= SaveValue(ShowNavaid, ProfileKeys::NOTAMShowNavaid, computer.notam.show_navaid);
  changed |= SaveValue(ShowObstacle, ProfileKeys::NOTAMShowObstacle, computer.notam.show_obstacle);
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
