// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include <fcntl.h>

#ifdef HAVE_MSVCRT

/* don't allow Windows to cripple binary files */
int _fmode = _O_BINARY;

#endif
