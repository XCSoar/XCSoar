/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Dialogs.h"
#include "Airspace/AirspacePolygon.hpp"
#include "Airspace/AirspaceCircle.hpp"
#include "Airspace/AirspaceVisitor.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/AirspaceWarningVisitor.hpp"
#include "Airspace/AirspaceVisibility.hpp"
#include "Airspace/Airspaces.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"

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
  AirspaceMapVisible(const SETTINGS_COMPUTER& _settings, 
                     const fixed& _altitude, const bool& _border,
                     const AirspaceWarningCopy2 &warnings):
    AirspaceVisible(_settings, _altitude),
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
public:
  AirspaceDetailsDialogVisitor(const GeoPoint &location):
    m_airspace(NULL),
    m_location(location) {}

  void Visit(const AirspacePolygon& as) {
    visit_general(as);
  }

  void Visit(const AirspaceCircle& as) {
    visit_general(as);
  }

  void visit_general(const AbstractAirspace& as) {
    if (as.inside(m_location))
      m_airspace = &as;
  }

  void display() {
    if (m_airspace)
      dlgAirspaceDetails(*m_airspace);
  }

  bool found() const {
    return m_airspace != NULL;
  }

private:
  const AbstractAirspace *m_airspace;
  const GeoPoint &m_location;
};

bool
GlueMapWindow::AirspaceDetailsAtPoint(const GeoPoint &location) const
{
  if (airspace_database == NULL)
    return false;

  AirspaceWarningCopy2 awc;
  if (airspace_warnings != NULL)
    airspace_warnings->visit_warnings(awc);

  AirspaceDetailsDialogVisitor airspace_copy_popup(location);
  const AirspaceMapVisible visible(SettingsComputer(),
                                   Basic().GetAltitudeBaroPreferred(),
                                   false, awc);

  airspace_database->visit_within_range(location, fixed(100.0),
                                        airspace_copy_popup, visible);

  airspace_copy_popup.display();

  return airspace_copy_popup.found();
}
