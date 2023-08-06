// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Listener.hpp"

#include <cassert>

class ObservationZone;

class ObservationZoneEditWidget
  : public RowFormWidget, protected DataFieldListener {

public:
  struct Listener {
    virtual void OnModified(ObservationZoneEditWidget &widget) noexcept = 0;
  };

private:
  ObservationZone &oz;

  Listener *listener = nullptr;

public:
  ObservationZoneEditWidget(ObservationZone &_oz) noexcept;

  void SetListener(Listener *_listener) noexcept {
    assert(listener == nullptr);
    assert(_listener != nullptr);

    listener = _listener;
  }

protected:
  const ObservationZone &GetObject() const noexcept {
    return oz;
  }

  ObservationZone &GetObject() noexcept {
    return oz;
  }

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};
