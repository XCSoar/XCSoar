/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_OBSERVATION_ZONE_EDIT_WIDGET_HPP
#define XCSOAR_OBSERVATION_ZONE_EDIT_WIDGET_HPP

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"

#include <assert.h>

class ObservationZone;

class ObservationZoneEditWidget
  : public RowFormWidget, protected DataFieldListener {

public:
  struct Listener {
    virtual void OnModified(ObservationZoneEditWidget &widget) = 0;
  };

private:
  ObservationZone &oz;

  Listener *listener;

public:
  ObservationZoneEditWidget(ObservationZone &_oz);

  void SetListener(Listener *_listener) {
    assert(listener == nullptr);
    assert(_listener != nullptr);

    listener = _listener;
  }

protected:
  const ObservationZone &GetObject() const {
    return oz;
  }

  ObservationZone &GetObject() {
    return oz;
  }

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) override;
};

#endif
