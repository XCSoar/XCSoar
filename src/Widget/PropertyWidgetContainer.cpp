// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PropertyWidgetContainer.hpp"
#include "Form/Edit.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Screen/Layout.hpp"
#include "ui/window/Window.hpp"

PropertyWidgetContainer::~PropertyWidgetContainer() noexcept = default;

void
PropertyWidgetContainer::CalculateLayout(const PixelRect &rc) noexcept
{
  const unsigned prop_height = Layout::GetMinimumControlHeight();
  const unsigned spacing = Layout::GetTextPadding();

  property_rect = rc;
  property_rect.bottom = property_rect.top + prop_height;

  content_rect = rc;
  content_rect.top = property_rect.bottom + spacing;
  content_rect.bottom = rc.bottom;
}

void
PropertyWidgetContainer::UpdatePropertyText(const char *text) noexcept
{
  if (property != nullptr)
    property->SetText(text);
}

PixelSize
PropertyWidgetContainer::GetMinimumSize() const noexcept
{
  return GetContentWidget().GetMinimumSize();
}

PixelSize
PropertyWidgetContainer::GetMaximumSize() const noexcept
{
  return GetContentWidget().GetMaximumSize();
}

void
PropertyWidgetContainer::Initialise([[maybe_unused]] ContainerWindow &parent,
                                    [[maybe_unused]] const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  if (!property)
    property = std::make_unique<WndProperty>(look);
}

void
PropertyWidgetContainer::Prepare(ContainerWindow &parent,
                                 const PixelRect &rc) noexcept
{
  CalculateLayout(rc);

  const DialogLook &look = UIGlobals::GetDialogLook();
  if (property)
    property->Create(parent, property_rect, property_label, 0, WindowStyle());
  else
    property = std::make_unique<WndProperty>(look);
  property->SetReadOnly(true);

  GetContentWidget().Prepare(parent, content_rect);
}

void
PropertyWidgetContainer::Unprepare() noexcept
{
  GetContentWidget().Unprepare();
  property.reset();
}

bool
PropertyWidgetContainer::Save(bool &changed) noexcept
{
  return GetContentWidget().Save(changed);
}

bool
PropertyWidgetContainer::Click() noexcept
{
  return GetContentWidget().Click();
}

void
PropertyWidgetContainer::ReClick() noexcept
{
  GetContentWidget().ReClick();
}

void
PropertyWidgetContainer::Show(const PixelRect &rc) noexcept
{
  CalculateLayout(rc);
  if (property)
    property->MoveAndShow(property_rect);
  GetContentWidget().Show(content_rect);
}

bool
PropertyWidgetContainer::Leave() noexcept
{
  return GetContentWidget().Leave();
}

void
PropertyWidgetContainer::Hide() noexcept
{
  if (property)
    property->Hide();
  GetContentWidget().Hide();
}

void
PropertyWidgetContainer::Move(const PixelRect &rc) noexcept
{
  CalculateLayout(rc);
  if (property && property->IsDefined() && property->IsVisible())
    property->Move(property_rect);
  GetContentWidget().Move(content_rect);
}

bool
PropertyWidgetContainer::SetFocus() noexcept
{
  return GetContentWidget().SetFocus();
}

bool
PropertyWidgetContainer::HasFocus() const noexcept
{
  return GetContentWidget().HasFocus();
}

bool
PropertyWidgetContainer::KeyPress(unsigned key_code) noexcept
{
  return GetContentWidget().KeyPress(key_code);
}
