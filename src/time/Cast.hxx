// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "FloatDuration.hxx"

template<class Rep, class Period>
constexpr auto
ToFloatSeconds(const std::chrono::duration<Rep,Period> &d) noexcept
{
	return std::chrono::duration_cast<FloatDuration>(d).count();
}
