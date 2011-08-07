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

#include "GlueMapWindow.hpp"
#include "Util/StaticArray.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Dialogs/ListPicker.hpp"
#include "Dialogs/Airspace.hpp"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/AirspaceWarningVisitor.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "NMEA/Aircraft.hpp"
#include "Language/Language.hpp"

#include <algorithm>

class AirspaceWarningCopy2 : public AirspaceWarningVisitor
{
public:
  void Visit(const AirspaceWarning& as) {
    if (as.get_warning_state() == AirspaceWarning::WARNING_INSIDE)
      ids_inside.checked_append(&as.get_airspace());
    else if (as.get_warning_state() > AirspaceWarning::WARNING_CLEAR)
      ids_warning.checked_append(&as.get_airspace());

    if (!as.get_ack_expired())
      ids_acked.checked_append(&as.get_airspace());
  }

  bool is_warning(const AbstractAirspace& as) const {
    return find(as, ids_warning);
  }

  bool is_acked(const AbstractAirspace& as) const {
    return find(as, ids_acked);
  }

  bool is_inside(const AbstractAirspace& as) const {
    return find(as, ids_inside);
  }

private:
  bool find(const AbstractAirspace& as, 
            const StaticArray<const AbstractAirspace *,64> &list) const {
    return list.contains(&as);
  }

  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning, ids_acked;
};

class AirspaceMapVisible: public AirspaceVisible
{
public:
  AirspaceMapVisible(const AirspaceComputerSettings &_computer_settings,
                     const AirspaceRendererSettings &_renderer_settings,
                     const AircraftState& _state, const bool& _border,
                     const AirspaceWarningCopy2 &warnings):
    AirspaceVisible(_computer_settings, _renderer_settings, _state),
    m_border(_border),
    m_warnings(warnings) {}

  virtual bool operator()(const AbstractAirspace& airspace) const {
    return condition(airspace);
  }

  bool condition(const AbstractAirspace& airspace) const {
    return parent_condition(airspace) 
      || m_warnings.is_inside(airspace)
      || m_warnings.is_warning(airspace);
  }

private:
  const bool &m_border;
  const AirspaceWarningCopy2 &m_warnings;
};

/**
 * Class to display airspace details dialog
 */
class AirspaceDetailsDialogVisitor: 
  public AirspaceVisitor
{
  static const AirspaceDetailsDialogVisitor *instance;
  SingleWindow &parent_window;
  StaticArray<const AbstractAirspace *, 32> airspaces;

public:
  AirspaceDetailsDialogVisitor(SingleWindow &_parent_window,
                               const GeoPoint &location)
    :parent_window(_parent_window),
    m_location(location) {}

  void Visit(const AirspacePolygon& as) {
    visit_general(as);
  }

  void Visit(const AirspaceCircle& as) {
    visit_general(as);
  }

  void visit_general(const AbstractAirspace& as) {
    if (as.inside(m_location))
      airspaces.checked_append(&as);
  }

  void sort() {
    std::sort(airspaces.begin(), airspaces.end(), CompareAirspaceBase);
  }

  void display() {
    if (airspaces.empty())
      return;

    switch (airspaces.size()) {
    case 0:
      /* no (visible) airspace here */
      break;

    case 1:
      /* only one airspace, show it */
      dlgAirspaceDetails(*airspaces[0]);
      break;

    default:
      /* more than one airspace: show a list */
      instance = this;
      unsigned line_height = Fonts::MapBold.get_height() + Layout::Scale(6) +
                             Fonts::MapLabel.get_height();
      int i = ListPicker(parent_window, _("Airspaces at this location"),
                         airspaces.size(), 0, line_height, PaintListItem);
      assert(i >= -1 && i < (int)airspaces.size());
      if (i >= 0)
        dlgAirspaceDetails(*airspaces[i]);
    }
  }

  bool found() const {
    return !airspaces.empty();
  }

private:
  const GeoPoint &m_location;

  static bool CompareAirspaceBase(const AbstractAirspace *a,
                                  const AbstractAirspace *b) {
    return AIRSPACE_ALT::SortHighest(a->get_base(), b->get_base());
  }

  static void PaintListItem(Canvas &canvas, const PixelRect rc, unsigned idx) {
    const AbstractAirspace &airspace = *instance->airspaces[idx];

    const Font &name_font = Fonts::MapBold;
    const Font &small_font = Fonts::MapLabel;

    canvas.select(name_font);
    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + Layout::FastScale(2), rc,
                        airspace.GetName());

    canvas.select(small_font);
    canvas.text_clipped(rc.left + Layout::FastScale(2),
                        rc.top + name_font.get_height() + Layout::FastScale(4),
                        rc, airspace.get_type_text(true));

    PixelSize size = canvas.text_size(_T("9999 m AGL"));
    unsigned altitude_width = size.cx;
    unsigned altitude_height = size.cy;

    canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                        rc.top + Layout::FastScale(2), rc,
                        airspace.get_top_text(true).c_str());

    canvas.text_clipped(rc.right - altitude_width - Layout::FastScale(4),
                        rc.top + altitude_height + Layout::FastScale(4) / 2,
                        rc, airspace.get_base_text(true).c_str());
  }
};

/* sorry about this ugly global variable.. */
const AirspaceDetailsDialogVisitor *AirspaceDetailsDialogVisitor::instance;

bool
GlueMapWindow::AirspaceDetailsAtPoint(const GeoPoint &location)
{
  const Airspaces *airspace_database = airspace_renderer.GetAirspaces();
  const ProtectedAirspaceWarningManager *airspace_warnings =
    airspace_renderer.GetAirspaceWarnings();

  if (airspace_database == NULL)
    return false;

  AirspaceWarningCopy2 awc;
  if (airspace_warnings != NULL)
    airspace_warnings->visit_warnings(awc);

  AirspaceDetailsDialogVisitor airspace_copy_popup(*(SingleWindow *)get_root_owner(),
                                                   location);
  const AirspaceMapVisible visible(SettingsComputer().airspace,
                                   SettingsMap().airspace,
                                   ToAircraftState(Basic(), Calculated()),
                                   false, awc);

  airspace_database->visit_within_range(location, fixed(100.0),
                                        airspace_copy_popup, visible);

  airspace_copy_popup.sort();
  airspace_copy_popup.display();

  return airspace_copy_popup.found();
}
