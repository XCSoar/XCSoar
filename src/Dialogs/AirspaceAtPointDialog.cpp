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

static const AirspaceLook *look;
static const AirspaceRendererSettings *settings;

class AirspaceWarningList
{
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

  bool is_warning(const AbstractAirspace& as) const {
    return ids_warning.contains(&as);
  }

  bool is_inside(const AbstractAirspace& as) const {
    return ids_inside.contains(&as);
  }

private:
  StaticArray<const AbstractAirspace *,64> ids_inside, ids_warning;
};

class AirspaceAtPointPredicate: public AirspaceVisiblePredicate
{
public:
  AirspaceAtPointPredicate(const AirspaceComputerSettings &_computer_settings,
                           const AirspaceRendererSettings &_renderer_settings,
                           const AircraftState& _state,
                           const AirspaceWarningList &warnings,
                           const GeoPoint _location):
    AirspaceVisiblePredicate(_computer_settings, _renderer_settings, _state),
    m_warnings(warnings), location(_location) {}

  bool condition(const AbstractAirspace& airspace) const {
    if (!AirspaceVisiblePredicate::condition(airspace) &&
        !m_warnings.is_inside(airspace) &&
        !m_warnings.is_warning(airspace))
      return false;

    return airspace.Inside(location);
  }

private:
  const AirspaceWarningList &m_warnings;
  const GeoPoint location;
};

/**
 * Class to display airspace details dialog
 */
class AirspaceDetailsDialogVisitor:
  public AirspaceVisitor
{
  static const AirspaceDetailsDialogVisitor *instance;
  StaticArray<const AbstractAirspace *, 32> airspaces;

public:
  void Visit(const AirspacePolygon& as) {
    visit_general(as);
  }

  void Visit(const AirspaceCircle& as) {
    visit_general(as);
  }

  void visit_general(const AbstractAirspace& as) {
    airspaces.checked_append(&as);
  }

  void sort() {
    std::sort(airspaces.begin(), airspaces.end(), CompareAirspaceBase);
  }

  void display(SingleWindow &parent_window) {
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

  static bool CompareAirspaceBase(const AbstractAirspace *a,
                                  const AbstractAirspace *b) {
    return AirspaceAltitude::SortHighest(a->GetBase(), b->GetBase());
  }

  static void PaintListItem(Canvas &canvas, const PixelRect rc, unsigned idx) {
    const unsigned line_height = rc.bottom - rc.top;

    const AbstractAirspace &airspace = *instance->airspaces[idx];

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
};

/* sorry about this ugly global variable.. */
const AirspaceDetailsDialogVisitor *AirspaceDetailsDialogVisitor::instance;

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

  const ProtectedAirspaceWarningManager *airspace_warnings =
    renderer.GetAirspaceWarnings();

  AirspaceWarningList awc;
  if (airspace_warnings != NULL)
    awc.Fill(*airspace_warnings);

  settings = &renderer_settings;
  look = &renderer.GetLook();

  AirspaceDetailsDialogVisitor airspace_copy_popup;
  const AirspaceAtPointPredicate visible(computer_settings, renderer_settings,
                                         ToAircraftState(basic, calculated), awc,
                                         location);

  airspace_database->visit_within_range(location, fixed(100.0),
                                        airspace_copy_popup, visible);

  airspace_copy_popup.sort();
  airspace_copy_popup.display(parent);

  return airspace_copy_popup.found();
}
