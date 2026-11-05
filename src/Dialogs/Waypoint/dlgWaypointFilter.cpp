// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/ListWidget.hpp"
#include "Profile/Profile.hpp"
#include "Renderer/WaypointRendererSettings.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "Language/Language.hpp"
#include "Engine/Waypoint/Waypoint.hpp"

#include <cassert>
#include <cstddef>

/**
 * SeeYou / OpenAIP CUP styles mapped by ParseStyle() — no thermal
 * hotspot (that type is not a CUP style).
 */
static constexpr Waypoint::Type FILTER_TYPES[] = {
  Waypoint::Type::NORMAL,
  Waypoint::Type::AIRFIELD,
  Waypoint::Type::OUTLANDING,
  Waypoint::Type::MOUNTAIN_PASS,
  Waypoint::Type::MOUNTAIN_TOP,
  Waypoint::Type::OBSTACLE,
  Waypoint::Type::VOR,
  Waypoint::Type::NDB,
  Waypoint::Type::TOWER,
  Waypoint::Type::DAM,
  Waypoint::Type::TUNNEL,
  Waypoint::Type::BRIDGE,
  Waypoint::Type::POWERPLANT,
  Waypoint::Type::CASTLE,
  Waypoint::Type::INTERSECTION,
  Waypoint::Type::MARKER,
  Waypoint::Type::REPORTING_POINT,
  Waypoint::Type::PGTAKEOFF,
  Waypoint::Type::PGLANDING,
};

static constexpr unsigned FILTER_TYPE_COUNT = std::size(FILTER_TYPES);

/** Extra row: airports without a 4-character (ICAO) short name. */
static constexpr unsigned NON_ICAO_AIRPORTS_ROW = FILTER_TYPE_COUNT;

static constexpr unsigned FILTER_ROW_COUNT = FILTER_TYPE_COUNT + 1;

static const char *
GetFilterTypeName(Waypoint::Type type) noexcept
{
  switch (type) {
  case Waypoint::Type::NORMAL:
    return _("Turnpoint");
  case Waypoint::Type::AIRFIELD:
    return _("Airport");
  case Waypoint::Type::OUTLANDING:
    return _("Landable");
  case Waypoint::Type::MOUNTAIN_PASS:
    return _("Mountain Pass");
  case Waypoint::Type::MOUNTAIN_TOP:
    return _("Mountain Top");
  case Waypoint::Type::OBSTACLE:
    return _("Transmitter Mast");
  case Waypoint::Type::TOWER:
    return _("Tower");
  case Waypoint::Type::TUNNEL:
    return _("Tunnel");
  case Waypoint::Type::BRIDGE:
    return _("Bridge");
  case Waypoint::Type::POWERPLANT:
    return _("Power Plant");
  case Waypoint::Type::VOR:
    return _("VOR");
  case Waypoint::Type::NDB:
    return _("NDB");
  case Waypoint::Type::DAM:
    return _("Dam");
  case Waypoint::Type::CASTLE:
    return _("Castle");
  case Waypoint::Type::INTERSECTION:
    return _("Intersection");
  case Waypoint::Type::MARKER:
    return _("Marker");
  case Waypoint::Type::REPORTING_POINT:
    return _("Control Point");
  case Waypoint::Type::PGTAKEOFF:
    return _("PG Take Off");
  case Waypoint::Type::PGLANDING:
    return _("PG Landing Zone");
  case Waypoint::Type::THERMAL_HOTSPOT:
  case Waypoint::Type::COUNT:
    break;
  }

  return _("Unknown");
}

class WaypointFilterListWidget : public ListWidget {
  bool changed;
  TextRowRenderer row_renderer;

public:
  WaypointFilterListWidget() noexcept
    :changed(false) {}

  bool IsModified() const noexcept {
    return changed;
  }

  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    ListControl &list = CreateList(parent, look, rc,
                                   row_renderer.CalculateLayout(*look.list.font));
    list.SetLength(FILTER_ROW_COUNT);
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

void
WaypointFilterListWidget::OnPaintItem(Canvas &canvas, PixelRect rc,
                                      unsigned i) noexcept
{
  assert(i < FILTER_ROW_COUNT);
  const WaypointRendererSettings &settings =
    CommonInterface::GetMapSettings().waypoint;

  const bool display = i == NON_ICAO_AIRPORTS_ROW
    ? settings.display_non_icao_airports
    : settings.IsTypeDisplayed(FILTER_TYPES[i]);

  rc.right = display
    ? row_renderer.DrawRightColumn(canvas, rc, _("Display"))
    : row_renderer.PreviousRightColumn(canvas, rc, _("Display"));

  const char *name = i == NON_ICAO_AIRPORTS_ROW
    ? _("Non-ICAO airports")
    : GetFilterTypeName(FILTER_TYPES[i]);
  row_renderer.DrawTextRow(canvas, rc, name);
}

void
WaypointFilterListWidget::OnActivateItem(unsigned index) noexcept
{
  assert(index < FILTER_ROW_COUNT);

  WaypointRendererSettings &settings =
    CommonInterface::SetMapSettings().waypoint;

  if (index == NON_ICAO_AIRPORTS_ROW)
    settings.SaveNonIcaoAirportsDisplay(!settings.display_non_icao_airports);
  else {
    const Waypoint::Type type = FILTER_TYPES[index];
    settings.SaveTypeDisplay(type, !settings.IsTypeDisplayed(type));
  }

  changed = true;
  ActionInterface::SendMapSettings();
  GetList().Invalidate();
}

void
dlgWaypointFilterShowModal() noexcept
{
  TWidgetDialog<WaypointFilterListWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(),
           _("Waypoints"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget();

  dialog.ShowModal();

  if (dialog.GetWidget().IsModified())
    Profile::Save();
}
