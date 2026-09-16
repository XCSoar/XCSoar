// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigListPanel.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include "util/StaticString.hxx"

#include <vector>

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
                               bool &value) noexcept
{
  const unsigned item = GetItemCount();

  AddItem(caption, [this, item, &value](){
    value = IsItemChecked(item);

    /* other items may depend on this switch */
    Refresh();
  }, {.toggle = true, .checked = value, .help = help});
}

/**
 * Let the user pick a percentage, one choice per step.
 *
 * @return true if the value has changed
 */
static bool
PickPercent(const char *caption, const char *help,
            int min, int max, int step, int &value) noexcept
{
  const unsigned n = (max - min) / step + 1;

  std::vector<StaticString<8>> captions(n);
  std::vector<PickerChoice> choices(n);
  int current = -1;

  for (unsigned i = 0; i < n; ++i) {
    const int percent = min + step * i;
    captions[i].Format("%d %%", percent);
    choices[i] = {captions[i].c_str()};

    if (percent == value)
      current = i;
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || picked == current)
    return false;

  value = min + step * picked;
  return true;
}

void
ConfigListPanel::AddPercentItem(const char *caption, const char *help,
                                int min, int max, int step,
                                int &value) noexcept
{
  StaticString<8> percent;
  percent.Format("%d %%", value);

  AddItem(caption, [this, caption, help, min, max, step, &value](){
    if (PickPercent(caption, help, min, max, step, value))
      Refresh();
  }, {.value = percent.c_str(), .chevron = true});
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
