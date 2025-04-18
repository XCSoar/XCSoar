// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Gauge/GlueGaugeVario.hpp"
#include "Gauge/GaugeVario.hpp"
#include "Blackboard/LiveBlackboard.hpp"

void
GlueGaugeVario::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.Disable();

  SetWindow(std::make_unique<GaugeVario>(blackboard, parent, look,
                                         rc, style));
}

void
GlueGaugeVario::Show(const PixelRect &rc) noexcept
{
  WindowWidget::Show(rc);

  blackboard.AddListener(*this);
}

void
GlueGaugeVario::Hide() noexcept
{
  blackboard.RemoveListener(*this);

  WindowWidget::Hide();
}

void
GlueGaugeVario::OnGPSUpdate([[maybe_unused]] const MoreData &basic)
{
  ((GaugeVario &)GetWindow()).Invalidate();
}
