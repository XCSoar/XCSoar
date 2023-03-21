// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Boolean.hpp"
#include "ComboList.hpp"

ComboList
DataFieldBoolean::CreateComboList([[maybe_unused]] const TCHAR *reference) const noexcept
{
  ComboList combo_list;
  combo_list.Append(false, false_text);
  combo_list.Append(true, true_text);

  combo_list.current_index = GetValue();
  return combo_list;
}

void
DataFieldBoolean::SetFromCombo(int i, const TCHAR *) noexcept
{
  ModifyValue(i != 0);
}

const TCHAR *
DataFieldBoolean::GetAsString() const noexcept
{
  return mValue ? true_text : false_text;
}

void
DataFieldBoolean::Inc() noexcept
{
  ModifyValue(true);
}

void
DataFieldBoolean::Dec() noexcept
{
  ModifyValue(false);
}
