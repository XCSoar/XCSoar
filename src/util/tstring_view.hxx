// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <string_view>

#ifdef _UNICODE
using tstring_view = std::wstring_view;
#else
using tstring_view = std::string_view;
#endif
