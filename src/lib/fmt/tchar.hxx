// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <fmt/core.h>

#ifdef _UNICODE

#include <fmt/xchar.h>

using fmt_tstring_view = fmt::wstring_view;
using fmt_tformat_context = fmt::wformat_context;
using fmt_tformat_args = fmt::wformat_args;

#else

using fmt_tstring_view = fmt::string_view;
using fmt_tformat_context = fmt::format_context;
using fmt_tformat_args = fmt::format_args;

#endif
