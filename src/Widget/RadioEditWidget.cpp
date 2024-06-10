// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RadioEditWidget.hpp"

void RadioEditWidget::OnModified(DataField &df) noexcept
{
  OnFrequencyChanged(dynamic_cast<RadioFrequencyDataField &>(df).GetValue());
}

PixelSize
RadioEditWidget::GetMinimumSize() const noexcept
{
  return {2u * Layout::GetMinimumControlHeight(),
          Layout::GetMinimumControlHeight()};
}

PixelSize
RadioEditWidget::GetMaximumSize() const noexcept
{
  return {2u * Layout::GetMaximumControlHeight(),
          Layout::GetMaximumControlHeight()};
}

static constexpr std::array<PixelRect, 2>
CalculateLayout(const PixelRect &total_rc) noexcept
{
  PixelRect freq_rc(total_rc);
  PixelRect btn_rc(total_rc);

  int border = total_rc.left + (total_rc.GetWidth() / 2);
  freq_rc.right = border;
  btn_rc.left = border;

  return {
      freq_rc,
      btn_rc};
}

void RadioEditWidget::Prepare(ContainerWindow &parent,
                                     const PixelRect &total_rc) noexcept
{
  const auto rc = CalculateLayout(total_rc);

  WindowStyle style;
  style.TabStop();
  style.Hide();

  freq_edit = std::make_unique<WndProperty>(parent, look, _T(""), rc[0], 0, style);
  RadioFrequencyDataField *df = new RadioFrequencyDataField(GetCurrentFrequency(), this);
  freq_edit->SetDataField(df);
  list_button = std::make_unique<Button>(parent, look.button, _("Select from list"),
                                         rc[1], style, [this]()
                                         { OnOpenList(); });
}

void RadioEditWidget::Show(const PixelRect &total_rc) noexcept
{
  const auto rc = CalculateLayout(total_rc);

  freq_edit->MoveAndShow(rc[0]);
  list_button->MoveAndShow(rc[1]);
}

void RadioEditWidget::Hide() noexcept
{
  list_button->Hide();
  freq_edit->Hide();
}

void RadioEditWidget::Move(const PixelRect &total_rc) noexcept
{
  const auto rc = CalculateLayout(total_rc);

  freq_edit->Move(rc[0]);
  list_button->Move(rc[1]);
}

bool RadioEditWidget::SetFocus() noexcept
{
  list_button->SetFocus();
  return true;
}

bool RadioEditWidget::HasFocus() const noexcept
{
  return (list_button->HasFocus() || freq_edit->HasFocus());
}

void RadioEditWidget::UpdateFrequencyField(RadioFrequency freq)
{
  assert(freq_edit != nullptr);
  auto *df = freq_edit->GetDataField();
  dynamic_cast<RadioFrequencyDataField *>(df)->SetValue(freq);
  freq_edit->RefreshDisplay();
}
