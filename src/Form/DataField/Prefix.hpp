// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "String.hpp"

#include <functional>

class PrefixDataField final : public DataFieldString {
public:
  typedef std::function<const char *(const char *)> AllowedCharactersFunction;

private:
  AllowedCharactersFunction allowed_characters;

public:
  PrefixDataField(const char *value,
                  AllowedCharactersFunction _allowed_characters,
                  DataFieldListener *listener=nullptr) noexcept
    :DataFieldString(Type::PREFIX, value, listener),
     allowed_characters(_allowed_characters) {}

  PrefixDataField(const char *value=_T(""),
                  DataFieldListener *listener=nullptr) noexcept
    :DataFieldString(Type::PREFIX, value, listener) {}

  const AllowedCharactersFunction &GetAllowedCharactersFunction() const noexcept {
    return allowed_characters;
  }

  /* virtual methods from class DataField */
  void Inc() noexcept override;
  void Dec() noexcept override;
  const char *GetAsDisplayString() const noexcept override;

protected:
  [[gnu::pure]]
  const char *GetAllowedCharacters() const noexcept {
    return allowed_characters
      ? allowed_characters(_T(""))
      : _T("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  }
};
