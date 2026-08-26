// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "QuickMenuConfigPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Input/InputEvents.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Menu/ButtonLabel.hpp"
#include "Menu/MenuData.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "UIGlobals.hpp"
#include "UISettings.hpp"
#include "Widget/RowFormWidget.hpp"
#include "util/StringFormat.hpp"
#include "util/TruncateString.hpp"
#include "util/UTF8.hpp"

#include <cstddef>

enum ControlIndex {
  CUSTOM_MENU,
  FIRST_ITEM,
  ADD_COMMAND = FIRST_ITEM + UISettings::MAX_CUSTOM_QUICK_MENU,
};

class QuickMenuConfigPanel final
  : public RowFormWidget, DataFieldListener {
  unsigned visible_count = 1;

public:
  QuickMenuConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  bool Save(bool &changed) noexcept override;

private:
  void UpdateVisibility() noexcept;
  void OnAddCommand() noexcept;

  /* methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;
};

static void
FormatMenuChoiceLabel(const char *label, char *dest, size_t dest_size) noexcept
{
  if (dest_size == 0)
    return;

  char buffer[128];
  const auto expanded = ButtonLabel::Expand(label, std::span{buffer});
  const char *src = expanded.visible && expanded.text != nullptr
    ? expanded.text
    : label;

  /* Collapse newlines to spaces in a temp buffer, advancing by whole
     UTF-8 characters so CopyTruncateString never receives a split
     sequence. */
  char normalized[128];
  size_t j = 0;
  for (const char *p = src; *p != '\0' && j + 1 < sizeof(normalized);) {
    if (*p == '\n' || *p == '\r') {
      if (j > 0 && normalized[j - 1] != ' ')
        normalized[j++] = ' ';
      ++p;
      continue;
    }

    std::size_t len = SequenceLengthUTF8(*p);
    if (len == 0)
      len = 1;
    if (j + len >= sizeof(normalized))
      break;
    for (std::size_t i = 0; i < len; ++i)
      normalized[j++] = *p++;
  }
  normalized[j] = '\0';

  CopyTruncateString(dest, dest_size, normalized);
}

static void
FillQuickMenuChoices(DataFieldEnum &dfe) noexcept
{
  dfe.ClearChoices();
  dfe.AddChoice(0, _("(none)"));

  const Menu *menu = InputEvents::GetMenu("RemoteStick");
  if (menu == nullptr)
    return;

  for (unsigned i = 0; i < Menu::MAX_ITEMS; ++i) {
    const auto &item = (*menu)[i];
    if (!item.IsDefined() || item.label == nullptr)
      continue;

    char display[128];
    FormatMenuChoiceLabel(item.label, display, sizeof(display));
    if (display[0] == '\0')
      continue;

    dfe.AddChoice(i, display);
  }

  /* Keep "(none)" first; sort command labels A–Z. */
  if (dfe.Count() > 1)
    dfe.Sort(1);
}

void
QuickMenuConfigPanel::UpdateVisibility() noexcept
{
  const bool enabled = GetValueBoolean(CUSTOM_MENU);

  for (unsigned i = 0; i < UISettings::MAX_CUSTOM_QUICK_MENU; ++i)
    SetRowAvailable(FIRST_ITEM + i, enabled && i < visible_count);

  SetRowAvailable(ADD_COMMAND, enabled);
}

void
QuickMenuConfigPanel::OnAddCommand() noexcept
{
  if (!GetValueBoolean(CUSTOM_MENU))
    return;

  if (visible_count >= UISettings::MAX_CUSTOM_QUICK_MENU)
    return;

  LoadValueEnum(FIRST_ITEM + visible_count, 0u);
  ++visible_count;
  UpdateVisibility();
}

void
QuickMenuConfigPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(CUSTOM_MENU, df))
    UpdateVisibility();
}

void
QuickMenuConfigPanel::Prepare(ContainerWindow &parent,
                              const PixelRect &rc) noexcept
{
  const UISettings &settings = CommonInterface::GetUISettings();

  RowFormWidget::Prepare(parent, rc);

  AddBoolean(_("Custom menu"),
             _("When enabled, the Quick Menu shows only the commands "
               "selected below, in that order. When disabled, the full "
               "default Quick Menu is used; your selection is kept for "
               "when you turn this back on."),
             settings.custom_quick_menu, this);

  visible_count = settings.custom_quick_menu_count > 0
    ? settings.custom_quick_menu_count
    : 1;

  for (unsigned i = 0; i < UISettings::MAX_CUSTOM_QUICK_MENU; ++i) {
    char caption[8];
    StringFormat(caption, sizeof(caption), "%u", i + 1);

    WndProperty *wp = AddEnum(caption, nullptr, this);
    DataFieldEnum &dfe = *(DataFieldEnum *)wp->GetDataField();
    FillQuickMenuChoices(dfe);

    unsigned value = 0;
    if (i < settings.custom_quick_menu_count)
      value = settings.custom_quick_menu_items[i];
    dfe.SetValue(value);
    wp->RefreshDisplay();
  }

  AddButton(_("Add command"), [this]() {
    OnAddCommand();
  });

  UpdateVisibility();
}

bool
QuickMenuConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;
  UISettings &settings = CommonInterface::SetUISettings();

  changed |= SaveValue(CUSTOM_MENU, ProfileKeys::CustomQuickMenu,
                       settings.custom_quick_menu);

  /* Always persist the command list, even when Custom menu is off, so
     the selection returns when the user enables it again. */
  unsigned new_count = 0;
  uint8_t new_items[UISettings::MAX_CUSTOM_QUICK_MENU]{};

  for (unsigned i = 0; i < visible_count; ++i) {
    const unsigned location = GetValueEnum(FIRST_ITEM + i);
    if (location == 0 || location >= Menu::MAX_ITEMS)
      continue;

    new_items[new_count++] = (uint8_t)location;
  }

  if (new_count != settings.custom_quick_menu_count)
    changed = true;
  else {
    for (unsigned i = 0; i < new_count; ++i) {
      if (new_items[i] != settings.custom_quick_menu_items[i]) {
        changed = true;
        break;
      }
    }
  }

  if (changed) {
    settings.custom_quick_menu_count = new_count;
    for (unsigned i = 0; i < UISettings::MAX_CUSTOM_QUICK_MENU; ++i)
      settings.custom_quick_menu_items[i] =
        i < new_count ? new_items[i] : 0;

    Profile::Set(ProfileKeys::CustomQuickMenuCount, new_count);
    for (unsigned i = 0; i < UISettings::MAX_CUSTOM_QUICK_MENU; ++i) {
      char profile_key[32];
      StringFormat(profile_key, sizeof(profile_key),
                   "CustomQuickMenuItem%u", i);
      if (i < new_count)
        Profile::Set(profile_key, (unsigned)new_items[i]);
      else
        Profile::Set(profile_key, 0u);
    }
  }

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateQuickMenuConfigPanel()
{
  return std::make_unique<QuickMenuConfigPanel>();
}
