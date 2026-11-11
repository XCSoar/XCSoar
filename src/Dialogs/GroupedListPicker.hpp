// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <span>

struct StaticEnumChoice;
class FileDataField;

/** One choice for PickChoice(). */
struct PickerChoice {
  const char *caption;

  /**
   * An explanation of this choice, shown below the list while the
   * cursor is on it; nullptr for none.
   */
  const char *help = nullptr;
};

/**
 * Let the user pick one of several choices from a #GroupedListWidget
 * with check marks.  A tap on a choice takes it and closes the view,
 * like the combo picker does; Cancel keeps the current one.
 *
 * @param help an explanation of the setting, shown above the list;
 * nullptr for none
 * @param current the index of the choice which is checked now; -1
 * for none
 * @return the index of the choice, or -1 if the user has cancelled
 */
int
PickChoice(const char *caption, const char *help,
           std::span<const PickerChoice> choices, int current) noexcept;

/**
 * The translated caption of one value of an enumeration, for the
 * item which shows it; an empty string for a value which is not in
 * the list.
 */
[[gnu::pure]]
const char *
GetEnumCaption(const StaticEnumChoice *list, unsigned value) noexcept;

/**
 * Let the user pick one value of an enumeration, with the captions
 * and the explanations of the choices translated.
 *
 * @param list the choices, terminated by an entry without a caption
 * @return true if the value has changed
 */
bool
PickEnum(const char *caption, const char *help,
         const StaticEnumChoice *list, unsigned &value) noexcept;

/**
 * Let the user pick one of the files the field knows, or none.  A
 * tap on a file chooses it and closes the view, like the combo
 * picker does; the field is left alone if the user cancels.
 *
 * @param help an explanation of the setting, shown above the list
 */
void
PickFile(const char *caption, const char *help, FileDataField &df) noexcept;
