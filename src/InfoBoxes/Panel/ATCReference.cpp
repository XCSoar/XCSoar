// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ATCReference.hpp"
#include "Interface.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Formatter/GeoPointFormatter.hpp"
#include "util/Macros.hpp"
#include "Components.hpp"
#include "Engine/Waypoint/Waypoints.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "DataComponents.hpp"

enum Controls {
  WAYPOINT,
  LOCATION,
  RELOCATE,
  CLEAR,
};

class ATCReferencePanel : public RowFormWidget {
public:
  ATCReferencePanel() noexcept
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void UpdateValues() noexcept;

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

void
ATCReferencePanel::UpdateValues() noexcept
{
  const GeoPoint &location =
    CommonInterface::GetComputerSettings().poi.atc_reference;

  const auto waypoint = location.IsValid()
    ? data_components->waypoints->GetNearest(location, 100)
    : nullptr;

  SetText(WAYPOINT, waypoint != nullptr ? waypoint->name.c_str() : "---");

  const char *location_string;
  char buffer[64];
  if (location.IsValid()) {
    FormatGeoPoint(location, buffer, ARRAY_SIZE(buffer),
                   CommonInterface::GetUISettings().format.coordinate_format);
    location_string = buffer;
  } else
    location_string = "---";

  SetText(LOCATION, location_string);
}

void
ATCReferencePanel::Prepare(ContainerWindow &parent,
                           const PixelRect &rc) noexcept
{
  RowFormWidget::Prepare(parent, rc);

  AddReadOnly(_("Waypoint"));
  AddReadOnly(_("Location"));

  AddButton(_("Relocate"), [this](){
    auto &location = CommonInterface::SetComputerSettings().poi.atc_reference;
    auto waypoint = ShowWaypointListDialog(*data_components->waypoints,
                                           CommonInterface::Basic().location);
    if (waypoint != nullptr) {
      location = waypoint->location;
      UpdateValues();
    }
  });

  AddButton(_("Clear"), [this](){
    auto &location = CommonInterface::SetComputerSettings().poi.atc_reference;
    location.SetInvalid();
    UpdateValues();
  });

  UpdateValues();
}

std::unique_ptr<Widget>
LoadATCReferencePanel([[maybe_unused]] unsigned id)
{
  return std::make_unique<ATCReferencePanel>();
}
