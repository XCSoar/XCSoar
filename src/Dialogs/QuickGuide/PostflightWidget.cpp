// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PostflightWidget.hpp"
#include "QuickGuideLayoutContext.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "Dialogs/InternalLink.hpp"

PostflightWindow::PostflightWindow() noexcept
  : QuickGuideLinkWindow()
{
  link_rects.resize(static_cast<std::size_t>(LinkAction::COUNT));
}

unsigned
PostflightWindow::Layout(Canvas *canvas, const PixelRect &rc,
                         PostflightWindow *window) noexcept
{
  QuickGuideLayoutContext ctx(canvas, rc, window);

  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x,
    _("After your flight, there are several steps to check "
      "and complete."))) + ctx.margin;

  // 1. Download Flight Log
  ctx.DrawNumberedItem(1,
    _("Download flight logs from your connected NMEA device, such "
      "as a FLARM unit or another supported logger. "
      "List available logs and save them in XCSoarData/logs, "
      "from where they can be manually uploaded to other devices "
      "or platforms such as WeGlide."),
    LinkAction::FLIGHT_DOWNLOAD, _("Config → Devices → Flight download"));

  // 2. Flight Analysis
  ctx.DrawNumberedItem(2,
    _("Review statistical data from your flight such "
      "as your flight score, barograph, and glide "
      "polar analysis."),
    LinkAction::ANALYSIS, _("Info → Analysis"));

  // 3. Flight Status
  ctx.DrawNumberedItem(3,
    _("Check detailed statistics and timing "
      "information of your flight."),
    LinkAction::STATUS, _("Info → Info → Status"));

  // 4. Upload Flight (last item, remove trailing margin)
  ctx.DrawNumberedItem(4,
    _("Flights can be uploaded directly from XCSoar to WeGlide. "
      "For this, configure your WeGlide User ID and date of "
      "birth in the system setup. "
      "You can find your User ID on weglide.org under "
      "'My profile' by copying the numbers from the URL."),
    LinkAction::WEGLIDE, _("Config → System → Setup → WeGlide"));
  ctx.y -= ctx.margin;  // Remove last item's trailing margin

  return ctx.GetHeight();
}

void
PostflightWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear();
  Layout(&canvas, GetClientRect(), this);
}

bool
PostflightWindow::OnLinkActivated(std::size_t index) noexcept
{
  switch (static_cast<LinkAction>(index)) {
  case LinkAction::FLIGHT_DOWNLOAD:
    return HandleInternalLink("xcsoar://config/devices");
  case LinkAction::ANALYSIS:
    return HandleInternalLink("xcsoar://dialog/analysis");
  case LinkAction::STATUS:
    return HandleInternalLink("xcsoar://dialog/status");
  case LinkAction::WEGLIDE:
    return HandleInternalLink("xcsoar://config/weglide");
  case LinkAction::COUNT:
    break;
  }
  return false;
}

PixelSize
PostflightWidget::GetMinimumSize() const noexcept
{
  return {Layout::FastScale(200), Layout::FastScale(200)};
}

PixelSize
PostflightWidget::GetMaximumSize() const noexcept
{
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
