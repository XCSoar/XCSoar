/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#ifndef XCSOAR_VARIABLE_KEYHOLE_ZONE_EDIT_WIDGET_HPP
#define XCSOAR_VARIABLE_KEYHOLE_ZONE_EDIT_WIDGET_HPP

#include "ObservationZoneEditWidget.hpp"
#include "Engine/Task/ObservationZones/VariableKeyholeZone.hpp"

/**
 * The purpose of this class is to enter or edit the so called
 * 'VariableKeyholeZone' observation zone type.
 */
class VariableKeyholeZoneEditWidget : public ObservationZoneEditWidget
  {
public:
  /**
   * Ctor.
   * @param oz The observation zone definition.
   */
  VariableKeyholeZoneEditWidget(VariableKeyholeZone &_oz);

protected:
  /**
   * The const version of the observation zone.
   * @return The OZ for const operations.
   */
  const VariableKeyholeZone &GetObject() const
    {
    return (const VariableKeyholeZone &)ObservationZoneEditWidget::GetObject();
    }

  /**
   * The non const instance of the observation zone.
   * @return The OZ for non const operations.
   */
  VariableKeyholeZone &GetObject()
    {
    return (VariableKeyholeZone &)ObservationZoneEditWidget::GetObject();
    }

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  bool Save(bool &changed) override;
  };

#endif  // XCSOAR_VARIABLE_KEYHOLE_ZONE_EDIT_WIDGET_HPP
