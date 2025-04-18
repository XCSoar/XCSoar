// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <exception>
#include <tchar.h>

void
ShowError(std::exception_ptr e, const TCHAR *caption) noexcept;

void
ShowError(const TCHAR *msg, std::exception_ptr e,
          const TCHAR *caption) noexcept;
