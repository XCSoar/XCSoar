// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapItemListDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

namespace {

/**
 * The two rows added at the top of the map item list.  The switches
 * are kept here until OK.  Cancel leaves the settings as they were.
 *
 * The dialog floats over the map, sized to this list.
 */
class MapItemListSetup final {
  GroupedListWidget *list = nullptr;

  bool add_location;
  bool add_arrival_altitude;

public:
  MapItemListSetup() noexcept
    :add_location(CommonInterface::GetMapSettings().item_list.add_location),
     add_arrival_altitude(CommonInterface::GetMapSettings()
                          .item_list.add_arrival_altitude) {}

  void SetList(GroupedListWidget &_list) noexcept {
    list = &_list;
  }

  void Build() noexcept;
  void Commit() noexcept;

private:
  void AddSwitch(const char *caption, const char *help,
                 bool checked, bool &field) noexcept;
};

void
MapItemListSetup::AddSwitch(const char *caption, const char *help,
                            bool checked, bool &field) noexcept
{
  /* a tap on the label selects the row.  A tap on the switch flips
     it.  OK is what keeps the two switches. */
  list->AddItem(caption, [&field]{
    field = !field;
  }, {.toggle = true, .checked = checked, .help = help});
}

void
MapItemListSetup::Build() noexcept
{
  list->AddGroup(nullptr);

  AddSwitch(_("Show Location row"),
            _("If enabled a row at the top will be added showing you the "
              "distance and bearing to the location and the elevation."),
            add_location, add_location);

  AddSwitch(_("Show Arrival Altitude"),
            _("If enabled a row at the top will be added showing you the "
              "arrival altitude at the location."),
            add_arrival_altitude, add_arrival_altitude);
}

void
MapItemListSetup::Commit() noexcept
{
  MapSettings &settings = CommonInterface::SetMapSettings();

  Profile::Update(ProfileKeys::EnableLocationMapItem,
                  settings.item_list.add_location, add_location);
  Profile::Update(ProfileKeys::EnableArrivalAltitudeMapItem,
                  settings.item_list.add_arrival_altitude,
                  add_arrival_altitude);
}

} // namespace

void
ShowMapItemListSettingsDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto *list = new GroupedListWidget(look);
  MapItemListSetup setup;
  setup.SetList(*list);

  WidgetDialog dialog(WidgetDialog::Floating{}, UIGlobals::GetMainWindow(),
                      look, _("Map Item List Settings"), list);
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.PrepareFloatingList();
  setup.Build();
  dialog.RefitList();

  if (dialog.ShowModal() == mrOK)
    setup.Commit();
}
