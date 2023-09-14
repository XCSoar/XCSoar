// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "IntrusiveList.hxx"

#include <algorithm> // for std::find_if()

/**
 * A variant of #IntrusiveList which is sorted automatically.  There
 * are obvious scalability problems with this approach, so use with
 * care.
 */
template<typename T, typename Compare=typename T::Compare,
	 typename HookTraits=IntrusiveListBaseHookTraits<T>,
	 IntrusiveListOptions options=IntrusiveListOptions{}>
class IntrusiveSortedList
	: public IntrusiveList<T, HookTraits, options>
{
	using Base = IntrusiveList<T, HookTraits, options>;

	[[no_unique_address]]
	Compare compare;

public:
	constexpr IntrusiveSortedList() noexcept = default;
	IntrusiveSortedList(IntrusiveSortedList &&src) noexcept = default;

	using typename Base::reference;
	using Base::begin;
	using Base::end;

	void insert(reference item) noexcept {
		auto position = std::find_if(begin(), end(), [this, &item](const auto &other){
			return !compare(other, item);
		});

		Base::insert(position, item);
	}
};
