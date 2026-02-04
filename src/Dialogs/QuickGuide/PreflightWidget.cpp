// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PreflightWidget.hpp"
#include "QuickGuideLayoutContext.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Plane/PlaneDialogs.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"

PixelSize PreflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

unsigned
PreflightWindow::Layout(Canvas *canvas, const PixelRect &rc,
                        PreflightWindow *window) noexcept
{
  QuickGuideLayoutContext ctx(canvas, rc, window);

  const TCHAR *t0 = _("There are several things that should be set and "
                      "checked before each flight.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x, t0)) + ctx.margin;

  // 1. Checklist
  ctx.DrawNumber(1);
  StaticString<512> t1;
  t1 = _("It is possible to store a checklist.");
  t1 += _T(" ");
  t1 += _("To do this, an xcsoar-checklist.txt file must be added "
          "to the XCSoarData folder.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent,
                                 t1.c_str())) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::CHECKLIST,
                                _("Info → Checklist"))) + ctx.margin;

  // 2. Aircraft / Polar
  ctx.DrawNumber(2);
  const TCHAR *t2 = _("Select and activate the correct aircraft and polar "
                      "configuration, so that weight and performance are "
                      "accurate.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent, t2)) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::PLANE, _("Config → Plane"))) + ctx.margin;

  // 3. Flight
  ctx.DrawNumber(3);
  const TCHAR *t3 = _("Set flight parameters such as wing loading, bugs, "
                      "QNH and maximum temperature.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent, t3)) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::FLIGHT, _("Info → Flight"))) + ctx.margin;

  // 4. Wind
  ctx.DrawNumber(4);
  const TCHAR *t4 = _("Configure wind data manually or enable auto wind to "
                      "set speed and direction.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent, t4)) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::WIND, _("Info → Wind"))) + ctx.margin;

  // 5. Task
  ctx.DrawNumber(5);
  const TCHAR *t5 = _("Create a task so XCSoar can guide navigation and "
                      "provide return support.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent,
                                 t5)) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::TASK_MANAGER,
                                _("Nav → Task Manager"))) + ctx.margin;

  return ctx.GetHeight();
}

PixelSize PreflightWidget::GetMaximumSize() const noexcept {
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

void
PreflightWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  Layout(&canvas, rc, this);
}

PreflightWindow::PreflightWindow() noexcept
  : QuickGuideLinkWindow()
{
  const auto count = static_cast<std::size_t>(LinkAction::COUNT);
  link_rects.resize(count);
}

bool
PreflightWindow::HandleLink(LinkAction link) noexcept
{
  switch (link) {
  case LinkAction::CHECKLIST:
    dlgChecklistShowModal();
    return true;
  case LinkAction::PLANE:
    dlgPlanesShowModal();
    return true;
  case LinkAction::FLIGHT:
    dlgBasicSettingsShowModal();
    return true;
  case LinkAction::WIND:
    ShowWindSettingsDialog();
    return true;
  case LinkAction::TASK_MANAGER:
    dlgTaskManagerShowModal();
    return true;
  case LinkAction::COUNT:
    break;
  }

  return false;
}

bool
PreflightWindow::OnLinkActivated(std::size_t index) noexcept
{
  return HandleLink(static_cast<LinkAction>(index));
}
