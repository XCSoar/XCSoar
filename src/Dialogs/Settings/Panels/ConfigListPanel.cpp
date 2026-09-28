// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigListPanel.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"

#include <cmath>

ConfigListPanel::ConfigListPanel() noexcept
  :GroupedListWidget(UIGlobals::GetDialogLook()) {}

void
ConfigListPanel::Refresh() noexcept
{
  Clear();

  expert = UIGlobals::GetDialogSettings().expert;

  Fill();
  UpdateLayout();
}

void
ConfigListPanel::AddToggleItem(const char *caption, const char *help,
                               bool &value, const char *subtitle,
                               bool disabled) noexcept
{
  const unsigned item = GetItemCount();

  AddItem(caption, [this, item, &value](){
    value = IsItemChecked(item);

    /* other items may depend on this switch */
    Refresh();
  }, {.subtitle = subtitle, .toggle = true, .checked = value, .help = help,
      .disabled = disabled});
}

void
ConfigListPanel::AddPercentItem(const char *caption, const char *help,
                                int min, int max, int step,
                                int &value) noexcept
{
  StaticString<8> percent;
  percent.Format("%d %%", value);

  AddItem(caption, [this, caption, help, min, max, step, &value](){
    if (PickNumber(caption, help, min, max, step, value,
                   [](StaticString<32> &s, int v){ s.Format("%d %%", v); }))
      Refresh();
  }, {.value = percent.c_str(), .chevron = true});
}

/**
 * Let the user pick a vertical speed, one choice per step of the
 * unit of the user.
 *
 * @return true if the value has changed
 */
static bool
PickVerticalSpeed(const char *caption, const char *help,
                  double min, double max, double &value,
                  bool include_sign) noexcept
{
  const double step = GetUserVerticalSpeedStep();
  const double user_min = Units::ToUserVSpeed(min);
  const unsigned n =
    (unsigned)std::lround((Units::ToUserVSpeed(max) - user_min) / step) + 1;

  std::vector<BasicStringBuffer<char, 32>> captions(n);
  std::vector<PickerChoice> choices(n);

  for (unsigned i = 0; i < n; ++i) {
    const double user_value = user_min + i * step;
    captions[i] = FormatUserVerticalSpeed(Units::ToSysVSpeed(user_value),
                                          true, include_sign);
    choices[i] = {captions[i].c_str()};
  }

  /* the choice nearest to the value */
  const int current =
    std::clamp((int)std::lround((Units::ToUserVSpeed(value) - user_min)
                                / step),
               0, (int)n - 1);

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || picked == current)
    return false;

  value = Units::ToSysVSpeed(user_min + picked * step);
  return true;
}

void
ConfigListPanel::AddVerticalSpeedItem(const char *caption, const char *help,
                                      double min, double max, double &value,
                                      bool include_sign) noexcept
{
  AddItem(caption, [this, caption, help, min, max, &value, include_sign](){
    if (PickVerticalSpeed(caption, help, min, max, value, include_sign))
      Refresh();
  }, {.value = FormatUserVerticalSpeed(value, true, include_sign).c_str(),
      .chevron = true});
}

void
ConfigListPanel::Prepare(ContainerWindow &parent,
                         const PixelRect &rc) noexcept
{
  LoadSettings();
  Refresh();

  GroupedListWidget::Prepare(parent, rc);
}

void
ConfigListPanel::Show(const PixelRect &rc) noexcept
{
  /* the user level may have changed on the menu since the list was
     filled */
  if (expert != UIGlobals::GetDialogSettings().expert)
    Refresh();

  GroupedListWidget::Show(rc);
}
