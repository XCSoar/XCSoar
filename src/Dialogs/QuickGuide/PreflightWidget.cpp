// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PreflightWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Plane/PlaneDialogs.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"

#include <winuser.h>

PixelSize PreflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize PreflightWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(500) };
}

void
PreflightWidget::Initialise(ContainerWindow &parent,
                            const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<PreflightWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
PreflightWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();

  int margin = Layout::FastScale(10);
  int x = rc.left + margin;
  int x_indent = x + Layout::FastScale(17);
  int y = rc.top + margin;

  const DialogLook &look = UIGlobals::GetDialogLook();

  const Font &fontDefault = look.text_font;
  
  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(10), false, false, true));

  canvas.Select(fontDefault);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);

  const TCHAR *t0 = _("There are several things that should be set and "
                      "checked before each flight.");
  PixelRect t0_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t0_height = canvas.DrawFormattedText(t0_rc, t0, DT_LEFT);
  y += int(t0_height) + margin;

  // 1. Checklist
  canvas.DrawText({x, y}, _T("1.)"));
  StaticString<512> t1;
  t1 = _("It is possible to store a checklist.");
  t1 += _T(" ");
  t1 += _("To do this, an xcsoar-checklist.txt file must be added "
          "to the XCSoarData folder.");
  PixelRect t1_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t1_height = canvas.DrawFormattedText(t1_rc, t1.c_str(), DT_LEFT);
  y += int(t1_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l1 = _("Info → Checklist");
  PixelRect l1_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned l1_height = DrawLink(canvas, LinkAction::CHECKLIST, l1_rc, l1);
  canvas.Select(fontDefault);
  y += int(l1_height) + margin;


  // 2. Aircraft / Polar
  canvas.DrawText({x, y}, _T("2.)"));
  const TCHAR *t2 = _("Select and activate the correct aircraft and polar "
                      "configuration, so that weight and performance are "
                      "accurate.");
  PixelRect t2_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t2_height = canvas.DrawFormattedText(t2_rc, t2, DT_LEFT);
  y += int(t2_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l2 = _("Config → Plane");
  PixelRect l2_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned l2_height = DrawLink(canvas, LinkAction::PLANE, l2_rc, l2);
  canvas.Select(fontDefault);
  y += int(l2_height) + margin;

  // 3. Flight
  canvas.DrawText({x, y}, _T("3.)"));
  const TCHAR *t3 = _("Set flight parameters such as wing loading, bugs, "
                      "QNH and maximum temperature.");
  PixelRect t3_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t3_height = canvas.DrawFormattedText(t3_rc, t3, DT_LEFT);
  y += int(t3_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l3 = _("Info → Flight");
  PixelRect l3_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned l3_height = DrawLink(canvas, LinkAction::FLIGHT, l3_rc, l3);
  canvas.Select(fontDefault);
  y += int(l3_height) + margin;

  // 4. Wind
  canvas.DrawText({x, y}, _T("4.)"));
  const TCHAR *t4 = _("Configure wind data manually or enable auto wind to "
                      "set speed and direction.");
  PixelRect t4_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t4_height = canvas.DrawFormattedText(t4_rc, t4, DT_LEFT);
  y += int(t4_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l4 = _("Info → Wind");
  PixelRect l4_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned l4_height = DrawLink(canvas, LinkAction::WIND, l4_rc, l4);
  canvas.Select(fontDefault);
  y += int(l4_height) + margin;

  // 5. Task
  canvas.DrawText({x, y}, _T("5.)"));
  const TCHAR *t5 = _("Create a task so XCSoar can guide navigation and "
                      "provide return support.");
  PixelRect t5_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t5_height = canvas.DrawFormattedText(t5_rc, t5, DT_LEFT);
  y += int(t5_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l5 = _("Nav → Task Manager");
  PixelRect l5_rc{x_indent, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned l5_height = DrawLink(canvas, LinkAction::TASK_MANAGER, l5_rc, l5);
  canvas.Select(fontDefault);
  y += int(l5_height) + margin;
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

unsigned
PreflightWindow::DrawLink(Canvas &canvas, LinkAction link_action, PixelRect rc,
                          const TCHAR *text) noexcept
{
  return QuickGuideLinkWindow::DrawLink(canvas,
                                        static_cast<std::size_t>(link_action),
                                        rc, text);
}

bool
PreflightWindow::OnLinkActivated(std::size_t link_action) noexcept
{
  return HandleLink(static_cast<LinkAction>(link_action));
}
