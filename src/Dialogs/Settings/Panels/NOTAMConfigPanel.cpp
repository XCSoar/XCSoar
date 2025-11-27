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

enum ControlIndex {
#ifdef HAVE_HTTP
  EnableNOTAM,
  NOTAMRadius,
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
  bool Save(bool &changed) noexcept override;
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
             _("Display NOTAMs related to airspace restrictions and changes."),
             computer.notam.show_airspace);

  AddBoolean(_("Show Airport NOTAMs"),
             _("Display NOTAMs related to airports, runways, and taxiways."),
             computer.notam.show_airport);

  AddBoolean(_("Show Navigation Aid NOTAMs"),
             _("Display NOTAMs related to VOR, NDB, ILS, and other navigation aids."),
             computer.notam.show_navaid);
  SetExpertRow(ShowNavaid);

  AddBoolean(_("Show Obstacle NOTAMs"),
             _("Display NOTAMs related to obstacles, towers, and construction."),
             computer.notam.show_obstacle);

  AddBoolean(_("Show Military NOTAMs"),
             _("Display NOTAMs related to military exercises and operations."),
             computer.notam.show_military);
  SetExpertRow(ShowMilitary);

  AddBoolean(_("Show Other NOTAMs"),
             _("Display miscellaneous NOTAMs not covered by other categories."),
             computer.notam.show_other);
  SetExpertRow(ShowOther);
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
