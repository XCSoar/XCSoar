// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once


#ifdef _WIN32
// #include <codecvt>
#include <locale>
#include <string>
#include <string_view>

std::wstring UTF8ToWide(const std::string_view s) noexcept;

#endif