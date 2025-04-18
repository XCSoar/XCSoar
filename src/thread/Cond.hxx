// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#ifdef _WIN32

#include "WindowsCond.hxx"
class Cond : public WindowsCond {};

#else

#include <condition_variable>
using Cond = std::condition_variable;

#endif
