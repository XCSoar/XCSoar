// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TabWidget.hpp"
#include "Form/TabDisplay.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"

TabWidget::Layout::Layout(Orientation orientation, PixelRect rc,
                          const TabDisplay &td, const Widget *e) noexcept
  :pager(rc)
{
  vertical = IsVertical(orientation, rc);

  if (vertical) {
    unsigned tab_width = td.GetRecommendedColumnWidth();
    if (e != nullptr) {
      const auto extra_size = e->GetMinimumSize();
      if (extra_size.width > tab_width)
        tab_width = extra_size.width;
    }

    tab_display = pager.CutLeftSafe(tab_width);

    if (e != nullptr) {
      auto max_size = e->GetMaximumSize();
      unsigned extra_height = max_size.height;
      unsigned max_height = rc.GetHeight() / 2;
      if (extra_height > max_height)
        extra_height = max_height;

      extra = tab_display.CutTopSafe(extra_height);
    }
  } else {
    unsigned tab_height = td.GetRecommendedRowHeight();
    if (e != nullptr) {
      const auto extra_size = e->GetMinimumSize();
      if (extra_size.height > tab_height)
        tab_height = extra_size.height;
    }

    tab_display = pager.CutTopSafe(tab_height);

    if (e != nullptr) {
      auto max_size = e->GetMaximumSize();
      unsigned extra_width = max_size.width;
      unsigned max_width = rc.GetWidth() / 3;
      if (extra_width > max_width)
        extra_width = max_width;

      extra = tab_display.CutLeftSafe(extra_width);
    }
  }
}

TabWidget::~TabWidget() noexcept
{
  delete tab_display;
}

void
TabWidget::LargeExtra() noexcept
{
  assert(extra != nullptr);

  if (large_extra)
    return;

  large_extra = true;
  extra->Move(PagerWidget::GetPosition());
}

void
TabWidget::RestoreExtra() noexcept
{
  assert(extra != nullptr);

  if (!large_extra)
    return;

  large_extra = false;
  extra->Move(extra_position);
}

void
TabWidget::AddTab(std::unique_ptr<Widget> widget, const char *caption,
                  const MaskedIcon *icon) noexcept
{
  tab_display->Add(caption, icon);
  PagerWidget::Add(std::move(widget));
}

const char *
TabWidget::GetButtonCaption(unsigned i) const noexcept
{
  return tab_display->GetCaption(i);
}

bool
TabWidget::ClickPage(unsigned i) noexcept
{
  if (!PagerWidget::ClickPage(i))
    return false;

  /* switching to a new page by mouse click focuses the first control
     of the page, which is important for devices without touch
     screen */
  PagerWidget::SetFocus();
  return true;
}

bool
TabWidget::NextPage() noexcept
{
  return Next(HasPointer());
}

bool
TabWidget::PreviousPage() noexcept
{
  return Previous(HasPointer());
}

PixelSize
TabWidget::GetMinimumSize() const noexcept
{
  auto size = PagerWidget::GetMinimumSize();
  if (tab_display != nullptr) {
    if (tab_display->IsVertical())
      size.width += tab_display->GetRecommendedColumnWidth();
    else
      size.height += tab_display->GetRecommendedRowHeight();
  }

  return size;
}

PixelSize
TabWidget::GetMaximumSize() const noexcept
{
  auto size = PagerWidget::GetMaximumSize();
  if (tab_display != nullptr) {
    if (tab_display->IsVertical())
      size.width += tab_display->GetRecommendedColumnWidth();
    else
      size.height += tab_display->GetRecommendedRowHeight();
  }

  return size;
}

void
TabWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();

  tab_display = new TabDisplay(*this, UIGlobals::GetDialogLook(),
                               parent, rc, Layout::IsVertical(orientation, rc),
                               style);

  if (extra != nullptr)
    extra->Initialise(parent, rc);

  PagerWidget::Initialise(parent, rc);
}

void
TabWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  const Layout layout(orientation, rc, *tab_display, extra.get());

  tab_display->UpdateLayout(layout.tab_display, layout.vertical);

  PagerWidget::Prepare(parent, layout.pager);

  if (extra != nullptr) {
    extra_position = layout.extra;
    extra->Prepare(parent, GetEffectiveExtraPosition());
  }
}

void
TabWidget::Unprepare() noexcept
{
  if (extra != nullptr)
    extra->Unprepare();

  PagerWidget::Unprepare();
}

void
TabWidget::Show(const PixelRect &rc) noexcept
{
  const Layout layout(orientation, rc, *tab_display, extra.get());

  tab_display->UpdateLayout(layout.tab_display, layout.vertical);
  tab_display->Show();

  PagerWidget::Show(layout.pager);

  if (extra != nullptr) {
    extra_position = layout.extra;
    extra->Show(GetEffectiveExtraPosition());
  }
}

void
TabWidget::Hide() noexcept
{
  PagerWidget::Hide();

  if (extra != nullptr)
    extra->Hide();

  tab_display->Hide();
}

void
TabWidget::Move(const PixelRect &rc) noexcept
{
  const Layout layout(orientation, rc, *tab_display, extra.get());

  tab_display->UpdateLayout(layout.tab_display, layout.vertical);

  PagerWidget::Move(layout.pager);

  if (extra != nullptr) {
    extra_position = layout.extra;
    extra->Move(GetEffectiveExtraPosition());
  }
}

bool
TabWidget::SetFocus() noexcept
{
  if (!PagerWidget::SetFocus())
    tab_display->SetFocus();

  return true;
}

bool
TabWidget::HasFocus() const noexcept
{
  return PagerWidget::HasFocus() || tab_display->HasFocus();
}

bool
TabWidget::KeyPress(unsigned key_code) noexcept
{
  // TODO: implement a few hotkeys

  return PagerWidget::KeyPress(key_code);
}

void
TabWidget::OnPageFlipped() noexcept
{
  tab_display->SetCurrentIndex(GetCurrentIndex());
  PagerWidget::OnPageFlipped();
}
