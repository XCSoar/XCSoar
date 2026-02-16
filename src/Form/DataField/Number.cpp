// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Number.hpp"

NumberDataField::NumberDataField(Type type, bool support_combo,
                                 const char *_edit_format,
                                 const char *_display_format,
                                 DataFieldListener *listener) noexcept
  :DataField(type, support_combo, listener),
   edit_format(_edit_format), display_format(_display_format)
{
}

void
NumberDataField::SetFormat(const char *text) noexcept
{
  edit_format = text;
  display_format = text;
  display_format += _T(" %s") ;
}
