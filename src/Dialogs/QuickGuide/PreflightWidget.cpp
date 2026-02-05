// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PreflightWidget.hpp"
#include "QuickGuideLayoutContext.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Dialogs/InternalLink.hpp"

PreflightWindow::PreflightWindow() noexcept
  : QuickGuideLinkWindow()
{
  link_rects.resize(static_cast<std::size_t>(LinkAction::COUNT));
}

unsigned
PreflightWindow::Layout(Canvas *canvas, const PixelRect &rc,
                        PreflightWindow *window) noexcept
{
  QuickGuideLayoutContext ctx(canvas, rc, window);

  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x,
    _("There are several things that should be set and "
      "checked before each flight."))) + ctx.margin;

  // 1. Aircraft / Polar
  ctx.DrawNumberedItem(1,
    _("Select and activate the correct aircraft and polar "
      "configuration, so that weight and performance are accurate."),
    LinkAction::PLANE, _("Config → Plane"));

  // 2. Flight
  ctx.DrawNumberedItem(2,
    _("Set flight parameters such as wing loading, bugs, "
      "QNH and maximum temperature."),
    LinkAction::FLIGHT, _("Info → Flight"));

  // 3. Wind
  ctx.DrawNumberedItem(3,
    _("Configure wind data manually or enable auto wind to "
      "set speed and direction."),
    LinkAction::WIND, _("Info → Wind"));

  // 4. Task (last item, remove trailing margin)
  ctx.DrawNumberedItem(4,
    _("Create a task so XCSoar can guide navigation and "
      "provide return support."),
    LinkAction::TASK_MANAGER, _("Nav → Task Manager"));
  ctx.y -= ctx.margin;  // Remove last item's trailing margin

  return ctx.GetHeight();
}

void
PreflightWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear();
  Layout(&canvas, GetClientRect(), this);
}

bool
PreflightWindow::OnLinkActivated(std::size_t index) noexcept
{
  switch (static_cast<LinkAction>(index)) {
  case LinkAction::PLANE:
    return HandleInternalLink("xcsoar://config/planes");
  case LinkAction::FLIGHT:
    return HandleInternalLink("xcsoar://dialog/flight");
  case LinkAction::WIND:
    return HandleInternalLink("xcsoar://dialog/wind");
  case LinkAction::TASK_MANAGER:
    return HandleInternalLink("xcsoar://dialog/task");
  case LinkAction::COUNT:
    break;
  }
  return false;
}

PixelSize
PreflightWidget::GetMinimumSize() const noexcept
{
  return {Layout::FastScale(200), Layout::FastScale(200)};
}

PixelSize
PreflightWidget::GetMaximumSize() const noexcept
{
  PixelSize size = GetMinimumSize();
  size.width = Layout::FastScale(300);

  unsigned width = size.width;
  if (IsDefined())
    width = GetWindow().GetSize().width;

  const PixelRect measure_rc{PixelPoint{0, 0}, PixelSize{width, 0u}};
  const unsigned height = PreflightWindow::Layout(nullptr, measure_rc, nullptr);
  if (height > size.height)
    size.height = height;

  return size;
}

void
PreflightWidget::Initialise(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();
  auto w = std::make_unique<PreflightWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

bool
PreflightWidget::SetFocus() noexcept
{
  GetWindow().SetFocus();
  return true;
}
