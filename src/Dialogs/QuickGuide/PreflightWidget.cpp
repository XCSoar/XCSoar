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
#include "Renderer/TextRenderer.hpp"

#include <winuser.h>

PixelSize PreflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

unsigned
PreflightWindow::Layout(Canvas *canvas, const PixelRect &rc,
                        PreflightWindow *window) noexcept
{
  const int margin = Layout::FastScale(10);
  const int x = rc.left + margin;
  const int x_indent = x + Layout::FastScale(17);
  int y = rc.top + margin;

  const int right = rc.right - margin;
  const DialogLook &look = UIGlobals::GetDialogLook();
  const Font &fontDefault = look.text_font;

  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(10), false, false, true));

  TextRenderer renderer;

  if (canvas != nullptr) {
    canvas->Select(fontDefault);
    canvas->SetBackgroundTransparent();
    canvas->SetTextColor(COLOR_BLACK);
  }

  auto DrawTextBlock = [&](const Font &font, int left, const TCHAR *text,
                           unsigned format=DT_LEFT) {
    if (canvas != nullptr) {
      canvas->Select(font);
      PixelRect text_rc{left, y, right, rc.bottom};
      return canvas->DrawFormattedText(text_rc, text, format);
    }

    const int width = right > left ? right - left : 0;
    return renderer.GetHeight(font, width, text);
  };

  auto DrawLinkLine = [&](LinkAction link, const TCHAR *text) {
    if (canvas != nullptr && window != nullptr) {
      canvas->Select(fontMono);
      PixelRect link_rc{x_indent, y, right, rc.bottom};
      return window->DrawLink(*canvas, link, link_rc, text);
    }

    const int width = right > x_indent ? right - x_indent : 0;
    return renderer.GetHeight(fontMono, width, text);
  };

  const TCHAR *t0 = _("There are several things that should be set and "
                      "checked before each flight.");
  y += int(DrawTextBlock(fontDefault, x, t0)) + margin;

  // 1. Checklist
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("1.)"));
  StaticString<512> t1;
  t1 = _("It is possible to store a checklist.");
  t1 += _T(" ");
  t1 += _("To do this, an xcsoar-checklist.txt file must be added "
          "to the XCSoarData folder.");
  y += int(DrawTextBlock(fontDefault, x_indent, t1.c_str())) + margin / 2;
  const TCHAR *l1 = _("Info → Checklist");
  y += int(DrawLinkLine(LinkAction::CHECKLIST, l1)) + margin;

  // 2. Aircraft / Polar
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("2.)"));
  const TCHAR *t2 = _("Select and activate the correct aircraft and polar "
                      "configuration, so that weight and performance are "
                      "accurate.");
  y += int(DrawTextBlock(fontDefault, x_indent, t2)) + margin / 2;
  const TCHAR *l2 = _("Config → Plane");
  y += int(DrawLinkLine(LinkAction::PLANE, l2)) + margin;

  // 3. Flight
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("3.)"));
  const TCHAR *t3 = _("Set flight parameters such as wing loading, bugs, "
                      "QNH and maximum temperature.");
  y += int(DrawTextBlock(fontDefault, x_indent, t3)) + margin / 2;
  const TCHAR *l3 = _("Info → Flight");
  y += int(DrawLinkLine(LinkAction::FLIGHT, l3)) + margin;

  // 4. Wind
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("4.)"));
  const TCHAR *t4 = _("Configure wind data manually or enable auto wind to "
                      "set speed and direction.");
  y += int(DrawTextBlock(fontDefault, x_indent, t4)) + margin / 2;
  const TCHAR *l4 = _("Info → Wind");
  y += int(DrawLinkLine(LinkAction::WIND, l4)) + margin;

  // 5. Task
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("5.)"));
  const TCHAR *t5 = _("Create a task so XCSoar can guide navigation and "
                      "provide return support.");
  y += int(DrawTextBlock(fontDefault, x_indent, t5)) + margin / 2;
  const TCHAR *l5 = _("Nav → Task Manager");
  y += int(DrawLinkLine(LinkAction::TASK_MANAGER, l5)) + margin;

  return static_cast<unsigned>(y);
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
  auto w = std::make_unique<PreflightWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
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
