// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ObservationZoneEditWidget.hpp"
#include "UIGlobals.hpp"

ObservationZoneEditWidget::ObservationZoneEditWidget(ObservationZone &_oz) noexcept
  :RowFormWidget(UIGlobals::GetDialogLook()),
   oz(_oz) {}

void
ObservationZoneEditWidget::OnModified([[maybe_unused]] DataField &df) noexcept
{
  if (listener != nullptr)
    listener->OnModified(*this);
}
