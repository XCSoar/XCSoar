// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef HAVE_POSIX

/**
 * Launch a child process but don't wait for it to exit.
 */
bool
Start(const char *const*argv) noexcept;

template<typename... Args>
static inline bool
Start(const char *path, Args... args) noexcept
{
  const char *const argv[]{path, args..., nullptr};
  return Start(argv);
}

/**
 * Launch a child process and wait for it to exit.
 */
bool
Run(const char *const*argv) noexcept;

template<typename... Args>
static inline bool
Run(const char *path, Args... args) noexcept
{
  const char *const argv[]{path, args..., nullptr};
  return Run(argv);
}

#endif
