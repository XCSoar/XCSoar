// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include "SliceAllocator.hxx"

template<typename T, unsigned size>
SliceAllocator<T, size> GlobalSliceAllocator<T, size>::allocator;
