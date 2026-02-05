// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Base.hpp"
#include "Listener.hpp"
#include "ComboList.hpp"

#include <math.h>

DataField::DataField(Type _type, bool _supports_combolist,
                     DataFieldListener *_listener) noexcept
  :listener(_listener),
   supports_combolist(_supports_combolist), type(_type),
   item_help_enabled(false)
{
}

void
DataField::Modified() noexcept
{
  if (on_modified)
    on_modified();

  if (listener != nullptr)
    listener->OnModified(*this);
}

void
DataField::Inc() noexcept
{
}

void
DataField::Dec() noexcept
{
}

const char *
DataField::GetAsString() const noexcept
{
  return nullptr;
}

const char *
DataField::GetAsDisplayString() const noexcept
{
  return GetAsString();
}

ComboList
DataField::CreateComboList([[maybe_unused]] const char *reference) const noexcept
{
  return ComboList();
}
