// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PostflightWidget.hpp"
#include "QuickGuideLayoutContext.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"
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
#include "Look/DialogLook.hpp"

PixelSize PostflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

unsigned
PostflightWindow::Layout(Canvas *canvas, const PixelRect &rc,
                         PostflightWindow *window) noexcept
{
  QuickGuideLayoutContext ctx(canvas, rc, window);

  const TCHAR *t0 = _("After your flight, there are several steps to check "
                      "and complete.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x, t0)) + ctx.margin;

  // 1. Download Flight Log
  ctx.DrawNumber(1);
  StaticString<1024> t1;
  t1 = _("Download flight logs from your connected NMEA device, such "
         "as a FLARM unit or another supported logger.");
  t1 += _T(" ");
  t1 += _("List available logs and save them in XCSoarData/logs, "
          "from where they can be manually uploaded to other devices "
          "or platforms such as WeGlide.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent,
                                 t1.c_str())) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::FLIGHT_DOWNLOAD,
               _("Config → Devices → Flight download"))) + ctx.margin;

  // 2. Flight Analysis
  ctx.DrawNumber(2);
  const TCHAR *t2 = _("Review statistical data from your flight such "
                      "as your flight score, barograph, and glide "
                      "polar analysis.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent, t2)) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::ANALYSIS, _("Info → Analysis"))) + ctx.margin;

  // 3. Flight Status
  ctx.DrawNumber(3);
  const TCHAR *t3 = _("Check detailed statistics and timing "
                      "information of your flight.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent,
                                 t3)) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::STATUS,
                                _("Info → Info → Status"))) + ctx.margin;

  // 4. Upload Flight
  ctx.DrawNumber(4);
  StaticString<1024> t4;
  t4 = _("Flights can be uploaded directly from XCSoar to WeGlide.");
  t4 += _T(" ");
  t4 += _("For this, configure your WeGlide User ID and date of "
          "birth in the system setup.");
  t4 += _T(" ");
  t4 += _("You can find your User ID on weglide.org under "
          "'My profile' by copying the numbers from the URL.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x_indent,
                                 t4.c_str())) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::WEGLIDE,
               _("Config → System → Setup → WeGlide"))) + ctx.margin;

  return ctx.GetHeight();
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

bool
PostflightWindow::OnLinkActivated(std::size_t index) noexcept
{
  return HandleLink(static_cast<LinkAction>(index));
}
