/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Dialogs/AirspaceAtPointDialog.hpp"
#include "Util/StaticArray.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/Airspace.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "NMEA/Aircraft.hpp"
#include "Renderer/AirspaceRenderer.hpp"
#include "Language/Language.hpp"
#include "Renderer/AirspacePreviewRenderer.hpp"

#include <algorithm>

class AirspaceListBuilderVisitor;

static const AirspaceLook *look;
static const AirspaceRendererSettings *settings;

class AirspaceWarningList
{
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning;

public:
  void Add(const AirspaceWarning& as) {
    if (as.get_warning_state() == AirspaceWarning::WARNING_INSIDE)
      ids_inside.checked_append(&as.get_airspace());
    else if (as.get_warning_state() > AirspaceWarning::WARNING_CLEAR)
      ids_warning.checked_append(&as.get_airspace());
  }

  void Fill(const AirspaceWarningManager &awm) {
    for (AirspaceWarningManager::const_iterator i = awm.begin(),
           end = awm.end(); i != end; ++i)
      Add(*i);
  }

  void Fill(const ProtectedAirspaceWarningManager &awm) {
    const ProtectedAirspaceWarningManager::Lease lease(awm);
    Fill(lease);
  }

  bool ContainsWarning(const AbstractAirspace& as) const {
    return ids_warning.contains(&as);
  }

  bool ContainsInside(const AbstractAirspace& as) const {
    return ids_inside.contains(&as);
  }
};

class AirspaceWarningPredicate: public AirspacePredicate
{
  const AirspaceWarningList &warnings;

public:
  AirspaceWarningPredicate(const AirspaceWarningList &_warnings)
    :warnings(_warnings) {}

  bool condition(const AbstractAirspace& airspace) const {
    return warnings.ContainsInside(airspace) ||
           warnings.ContainsWarning(airspace);
  }
};

class AirspaceInsidePredicate: public AirspacePredicate
{
  const GeoPoint location;

public:
  AirspaceInsidePredicate(const GeoPoint _location)
    :location(_location) {}

  bool condition(const AbstractAirspace& airspace) const {
    return airspace.Inside(location);
  }
};

class AirspaceAtPointPredicate: public AirspacePredicate
{
  AirspaceVisiblePredicate visible_predicate;
  AirspaceWarningPredicate warning_predicate;
  AirspaceInsidePredicate inside_predicate;

public:
  AirspaceAtPointPredicate(const AirspaceComputerSettings &_computer_settings,
                           const AirspaceRendererSettings &_renderer_settings,
                           const AircraftState& _state,
                           const AirspaceWarningList &_warnings,
                           const GeoPoint _location)
    :visible_predicate(_computer_settings, _renderer_settings, _state),
     warning_predicate(_warnings),
     inside_predicate(_location) {}

  bool condition(const AbstractAirspace& airspace) const {
    // Airspace should be visible or have a warning/inside status
    // and airspace needs to be at specified location

    return (visible_predicate(airspace) || warning_predicate(airspace)) &&
           inside_predicate(airspace);
  }
};

/**
 * Class to display airspace details dialog
 */
class AirspaceListBuilderVisitor:
  public AirspaceVisitor
{
  StaticArray<const AbstractAirspace *, 32> airspaces;

public:
  void Visit(const AirspacePolygon& as) {
    airspaces.checked_append(&as);
  }

  void Visit(const AirspaceCircle& as) {
    airspaces.checked_append(&as);
  }

  void sort() {
    std::sort(airspaces.begin(), airspaces.end(), CompareAirspaceBase);
  }

  void clear() {
    airspaces.clear();
  }

  bool empty() const {
    return airspaces.empty();
  }

  unsigned size() const {
    return airspaces.size();
  }

  const AbstractAirspace *operator[](unsigned index) const {
    assert(index < airspaces.size());
    return airspaces[index];
  }

private:
  static bool CompareAirspaceBase(const AbstractAirspace *a,
                                  const AbstractAirspace *b) {
    return AirspaceAltitude::SortHighest(a->GetBase(), b->GetBase());
  }
};

static AirspaceListBuilderVisitor list;

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  const unsigned line_height = rc.bottom - rc.top;

  const AbstractAirspace &airspace = *list[idx];

  RasterPoint pt = { rc.left + line_height / 2,
                     rc.top + line_height / 2};
  unsigned radius = std::min(line_height / 2  - Layout::FastScale(4),
                             (unsigned)Layout::FastScale(10));
  AirspacePreviewRenderer::Draw(canvas, airspace, pt, radius,
                                *settings, *look);

  const Font &name_font = Fonts::MapBold;
  const Font &small_font = Fonts::MapLabel;

  unsigned left = rc.left + line_height + Layout::FastScale(2);
  canvas.select(name_font);
  canvas.text_clipped(left, rc.top + Layout::FastScale(2), rc,
                      airspace.GetName());

  canvas.select(small_font);
  canvas.text_clipped(left,
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, airspace.GetTypeText(false));

  unsigned altitude_width = canvas.text_width(airspace.GetTopText(true).c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.get_height() -
                      small_font.get_height() + Layout::FastScale(2), rc,
                      airspace.GetTopText(true).c_str());

  altitude_width = canvas.text_width(airspace.GetBaseText(true).c_str());
  canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                      rc.top + name_font.get_height() + Layout::FastScale(4),
                      rc, airspace.GetBaseText(true).c_str());
}

static void
ShowDialog(SingleWindow &parent)
{
  if (list.empty())
    return;

  switch (list.size()) {
  case 0:
    /* no (visible) airspace here */
    break;

  case 1:
    /* only one airspace, show it */
    dlgAirspaceDetails(*list[0]);
    break;

  default:
    /* more than one airspace: show a list */
    unsigned line_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                           Fonts::MapLabel.get_height();
    int i = ListPicker(parent, _("Airspaces at this location"),
                       list.size(), 0, line_height, PaintListItem);
    assert(i >= -1 && i < (int)list.size());
    if (i >= 0)
      dlgAirspaceDetails(*list[i]);
  }
}

bool
ShowAirspaceAtPointDialog(SingleWindow &parent, const GeoPoint &location,
                          const AirspaceRenderer &renderer,
                          const AirspaceComputerSettings &computer_settings,
                          const AirspaceRendererSettings &renderer_settings,
                          const MoreData &basic, const DerivedInfo &calculated)
{
  const Airspaces *airspace_database = renderer.GetAirspaces();
  if (airspace_database == NULL)
    return false;

  list.clear();

  const ProtectedAirspaceWarningManager *airspace_warnings =
    renderer.GetAirspaceWarnings();

  AirspaceWarningList warnings;
  if (airspace_warnings != NULL)
    warnings.Fill(*airspace_warnings);

  settings = &renderer_settings;
  look = &renderer.GetLook();

  AirspaceAtPointPredicate predicate(computer_settings, renderer_settings,
                                     ToAircraftState(basic, calculated),
                                     warnings, location);

  airspace_database->visit_within_range(location, fixed(100.0), list, predicate);
  list.sort();

  ShowDialog(parent);

  return !list.empty();
}
