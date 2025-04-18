// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RowFormWidget.hpp"
#include "Form/DataField/File.hpp"
#include "system/Path.hpp"

#include <cassert>

Path
RowFormWidget::GetValueFile(unsigned i) const noexcept
{
  const auto &df = (const FileDataField &)GetDataField(i);
  assert(df.GetType() == DataField::Type::FILE);
  return df.GetValue();
}

void
RowFormWidget::LoadValue(unsigned i, Path value) noexcept
{
  WndProperty &control = GetControl(i);
  FileDataField &df = *(FileDataField *)control.GetDataField();
  assert(df.GetType() == DataField::Type::FILE);
  df.SetValue(value);
  control.RefreshDisplay();
}
