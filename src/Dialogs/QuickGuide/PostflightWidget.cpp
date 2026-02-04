// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PostflightWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "MainWindow.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Settings/Panels/WeGlideConfigPanel.hpp"
#include "Profile/Profile.hpp"
#include "Renderer/TextRenderer.hpp"

#include <winuser.h>

PixelSize PostflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

unsigned
PostflightWindow::Layout(Canvas *canvas, const PixelRect &rc,
                         PostflightWindow *window) noexcept
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

  const TCHAR *t0 = _("After your flight, there are several steps to check "
                      "and complete.");
  y += int(DrawTextBlock(fontDefault, x, t0)) + margin;

  // 1. Download Flight Log
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("1.)"));
  StaticString<1024> t1;
  t1 = _("Download flight logs from your connected NMEA device, such "
         "as a FLARM unit or another supported logger.");
  t1 += _T(" ");
  t1 += _("List available logs and save them in XCSoarData/logs, "
          "from where they can be manually uploaded to other devices "
          "or platforms such as WeGlide.");
  y += int(DrawTextBlock(fontDefault, x_indent, t1.c_str())) + margin / 2;
  const TCHAR *l1 = _("Config → Devices → Flight download");
  y += int(DrawLinkLine(LinkAction::FLIGHT_DOWNLOAD, l1)) + margin;

  // 2. Flight Analysis
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("2.)"));
  const TCHAR *t2 = _("Review statistical data from your flight such "
                      "as your flight score, barograph, and glide "
                      "polar analysis.");
  y += int(DrawTextBlock(fontDefault, x_indent, t2)) + margin / 2;
  const TCHAR *l2 = _("Info → Analysis");
  y += int(DrawLinkLine(LinkAction::ANALYSIS, l2)) + margin;

  // 3. Flight Status
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("3.)"));
  const TCHAR *t3 = _("Check detailed statistics and timing "
                      "information of your flight.");
  y += int(DrawTextBlock(fontDefault, x_indent, t3)) + margin / 2;
  const TCHAR *l3 = _("Info → Info → Status");
  y += int(DrawLinkLine(LinkAction::STATUS, l3)) + margin;

  // 4. Upload Flight
  if (canvas != nullptr)
    canvas->DrawText({x, y}, _T("4.)"));
  StaticString<1024> t4;
  t4 = _("Flights can be uploaded directly from XCSoar to WeGlide.");
  t4 += _T(" ");
  t4 += _("For this, configure your WeGlide User ID and date of "
          "birth in the system setup.");
  t4 += _T(" ");
  t4 += _("You can find your User ID on weglide.org under "
          "'My profile' by copying the numbers from the URL.");
  y += int(DrawTextBlock(fontDefault, x_indent, t4.c_str())) + margin / 2;
  const TCHAR *l4 = _("Config → System → Setup → WeGlide");
  y += int(DrawLinkLine(LinkAction::WEGLIDE, l4)) + margin;

  return static_cast<unsigned>(y);
}

PixelSize PostflightWidget::GetMaximumSize() const noexcept {
  PixelSize size = GetMinimumSize();
  size.width = Layout::FastScale(300);

  unsigned width = size.width;
  if (IsDefined())
    width = GetWindow().GetSize().width;

  const PixelRect measure_rc{PixelPoint{0, 0}, PixelSize{width, 0u}};
  const unsigned height = PostflightWindow::Layout(nullptr, measure_rc, nullptr);
  if (height > size.height)
    size.height = height;

  return size;
}

void
PostflightWidget::Initialise(ContainerWindow &parent,
                             const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();
  auto w = std::make_unique<PostflightWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

bool
PostflightWidget::SetFocus() noexcept
{
  GetWindow().SetFocus();
  return true;
}

void
PostflightWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  Layout(&canvas, rc, this);
}

PostflightWindow::PostflightWindow() noexcept
  : QuickGuideLinkWindow()
{
  const auto count = static_cast<std::size_t>(LinkAction::COUNT);
  link_rects.resize(count);
}

bool
PostflightWindow::HandleLink(LinkAction link) noexcept
{
  switch (link) {
  case LinkAction::FLIGHT_DOWNLOAD:
    if (backend_components != nullptr &&
        backend_components->device_blackboard != nullptr) {
      ShowDeviceList(*backend_components->device_blackboard,
                     backend_components->devices.get());
      return true;
    }
    break;

  case LinkAction::ANALYSIS:
    dlgAnalysisShowModal(*CommonInterface::main_window,
                         CommonInterface::main_window->GetLook(),
                         CommonInterface::Full(),
                         *backend_components->glide_computer,
                         data_components->airspaces.get(),
                         data_components->terrain.get());
    return true;

  case LinkAction::STATUS:
    dlgStatusShowModal(-1);
    return true;

  case LinkAction::WEGLIDE: {
    const DialogLook &look = UIGlobals::GetDialogLook();
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("WeGlide"));
    auto panel = CreateWeGlideConfigPanel();
    dialog.FinishPreliminary(std::move(panel));
    dialog.AddButton(_("Close"), mrOK);
    dialog.ShowModal();
    if (dialog.GetChanged()) {
      Profile::Save();
    }
    return true;
  }

  case LinkAction::COUNT:
	break;
  }

  return false;
}

unsigned
PostflightWindow::DrawLink(Canvas &canvas, LinkAction link_action,
                           PixelRect rc, const TCHAR *text) noexcept
{
  return QuickGuideLinkWindow::DrawLink(canvas,
                                        static_cast<std::size_t>(link_action),
                                        rc, text);
}

bool
PostflightWindow::OnLinkActivated(std::size_t link_action) noexcept
{
  return HandleLink(static_cast<LinkAction>(link_action));
}
