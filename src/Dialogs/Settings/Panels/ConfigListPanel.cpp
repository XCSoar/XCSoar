// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigListPanel.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Math/Util.hpp"
#include "UIGlobals.hpp"
#include "Units/Units.hpp"

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
                               bool &value, const char *subtitle) noexcept
{
  const unsigned item = GetItemCount();

  AddItem(caption, [this, item, &value](){
    value = IsItemChecked(item);

    /* other items may depend on this switch */
    Refresh();
  }, {.subtitle = subtitle, .toggle = true, .checked = value, .help = help});
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

void
ConfigListPanel::AddAltitudeItem(const char *caption, const char *help,
                                 unsigned min, unsigned max, unsigned step,
                                 unsigned &value) noexcept
{
  AddItem(caption, [this, caption, help, min, max, step, &value](){
    int user_value = iround(Units::ToUserAltitude(value));
    if (PickNumber(caption, help, min, max, step, user_value,
                   [](StaticString<32> &s, int v){
                     s = FormatUserAltitude(Units::ToSysAltitude(v)).c_str();
                   })) {
      value = iround(Units::ToSysAltitude(user_value));
      Refresh();
    }
  }, {.value = FormatUserAltitude(value).c_str(), .chevron = true});
}

void
ConfigListPanel::AddDurationItem(const char *caption, const char *help,
                                 unsigned min, unsigned max, unsigned step,
                                 Duration &value) noexcept
{
  AddItem(caption, [this, caption, help, min, max, step, &value](){
    int seconds = value.count();
    if (PickNumber(caption, help, min, max, step, seconds,
                   [](StaticString<32> &s, int v){
                     s = FormatTimespanSmart(std::chrono::seconds{v},
                                             2).c_str();
                   })) {
      value = Duration{(unsigned)seconds};
      Refresh();
    }
  }, {.value = FormatTimespanSmart(std::chrono::seconds{value}, 2).c_str(),
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
