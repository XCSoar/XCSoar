// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Base.hpp"
#include "util/StaticString.hxx"

class NumberDataField : public DataField {
protected:
  StaticString<32> edit_format;
  StaticString<32> display_format;

public:
  void SetFormat(const TCHAR *text) noexcept;

protected:
  NumberDataField(Type type, bool support_combo,
                  const TCHAR *edit_format, const TCHAR *display_format,
                  DataFieldListener *listener=nullptr) noexcept;
};
