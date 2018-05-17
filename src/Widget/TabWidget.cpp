/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "TabWidget.hpp"
#include "Form/TabDisplay.hpp"
#include "UIGlobals.hpp"
#include "Asset.hpp"

TabWidget::Layout::Layout(Orientation orientation, PixelRect rc,
                          const TabDisplay &td, const Widget *e)
  :tab_display(rc), pager(rc)
{
  vertical = IsVertical(orientation, rc);

  if (vertical) {
    tab_display.right = pager.left = rc.left + td.GetRecommendedColumnWidth();

    if (e != nullptr) {
      auto max_size = e->GetMaximumSize();
      unsigned extra_height = max_size.cy;
      unsigned max_height = rc.GetHeight() / 2;
      if (extra_height > max_height)
        extra_height = max_height;

      extra = tab_display;
      tab_display.top = extra.bottom = extra.top + extra_height;
    }
  } else {
    tab_display.bottom = pager.top = rc.top + td.GetRecommendedRowHeight();

    if (e != nullptr) {
      auto max_size = e->GetMaximumSize();
      unsigned extra_width = max_size.cx;
      unsigned max_width = rc.GetWidth() / 3;
      if (extra_width > max_width)
        extra_width = max_width;

      extra = tab_display;
      tab_display.left = extra.right = extra.left + extra_width;
    }
  }
}

TabWidget::~TabWidget()
{
  delete tab_display;
  delete extra;
}

void
TabWidget::LargeExtra()
{
  assert(extra != nullptr);

  if (large_extra)
    return;

  large_extra = true;
  extra->Move(PagerWidget::GetPosition());
}

void
TabWidget::RestoreExtra()
{
  assert(extra != nullptr);

  if (!large_extra)
    return;

  large_extra = false;
  extra->Move(extra_position);
}

void
TabWidget::AddTab(Widget *widget, const TCHAR *caption,
                  const MaskedIcon *icon)
{
  tab_display->Add(caption, icon);
  PagerWidget::Add(widget);
}

const TCHAR *
TabWidget::GetButtonCaption(unsigned i) const
{
  return tab_display->GetCaption(i);
}

bool
TabWidget::ClickPage(unsigned i)
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
TabWidget::NextPage()
{
  return Next(HasPointer());
}

bool
TabWidget::PreviousPage()
{
  return Previous(HasPointer());
}

PixelSize
TabWidget::GetMinimumSize() const
{
  auto size = PagerWidget::GetMinimumSize();
  if (tab_display != nullptr) {
    if (tab_display->IsVertical())
      size.cx += tab_display->GetRecommendedColumnWidth();
    else
      size.cy += tab_display->GetRecommendedRowHeight();
  }

  return size;
}

PixelSize
TabWidget::GetMaximumSize() const
{
  auto size = PagerWidget::GetMaximumSize();
  if (tab_display != nullptr) {
    if (tab_display->IsVertical())
      size.cx += tab_display->GetRecommendedColumnWidth();
    else
      size.cy += tab_display->GetRecommendedRowHeight();
  }

  return size;
}

void
TabWidget::Initialise(ContainerWindow &parent, const PixelRect &rc)
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
TabWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Layout layout(orientation, rc, *tab_display, extra);

  tab_display->UpdateLayout(layout.tab_display, layout.vertical);

  PagerWidget::Prepare(parent, layout.pager);

  if (extra != nullptr) {
    extra_position = layout.extra;
    extra->Prepare(parent, GetEffectiveExtraPosition());
  }
}

void
TabWidget::Unprepare()
{
  if (extra != nullptr)
    extra->Unprepare();

  PagerWidget::Unprepare();
}

void
TabWidget::Show(const PixelRect &rc)
{
  const Layout layout(orientation, rc, *tab_display, extra);

  tab_display->UpdateLayout(layout.tab_display, layout.vertical);
  tab_display->Show();

  PagerWidget::Show(layout.pager);

  if (extra != nullptr) {
    extra_position = layout.extra;
    extra->Show(GetEffectiveExtraPosition());
  }
}

void
TabWidget::Hide()
{
  PagerWidget::Hide();

  if (extra != nullptr)
    extra->Hide();

  tab_display->Hide();
}

void
TabWidget::Move(const PixelRect &rc)
{
  const Layout layout(orientation, rc, *tab_display, extra);

  tab_display->UpdateLayout(layout.tab_display, layout.vertical);

  PagerWidget::Move(layout.pager);

  if (extra != nullptr) {
    extra_position = layout.extra;
    extra->Move(GetEffectiveExtraPosition());
  }
}

bool
TabWidget::SetFocus()
{
  if (!PagerWidget::SetFocus())
    tab_display->SetFocus();

  return true;
}

bool
TabWidget::KeyPress(unsigned key_code)
{
  // TODO: implement a few hotkeys

  return PagerWidget::KeyPress(key_code);
}

void
TabWidget::OnPageFlipped()
{
  tab_display->Invalidate();
  PagerWidget::OnPageFlipped();
}
