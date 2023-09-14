// SPDX-License-Identifier: BSD-2-Clause
// author: Max Kellermann <max.kellermann@gmail.com>

#pragma once

#include <curl/curl.h>

#include <chrono>
#include <stdexcept>
#include <utility>

/**
 * An OO wrapper for a "CURLM*" (a libCURL "multi" handle).
 */
class CurlMulti {
	CURLM *handle = nullptr;

public:
	/**
	 * Allocate a new CURLM*.
	 *
	 * Throws on error.
	 */
	CurlMulti()
		:handle(curl_multi_init())
	{
		if (handle == nullptr)
			throw std::runtime_error("curl_multi_init() failed");
	}

	/**
	 * Create an empty instance.
	 */
	CurlMulti(std::nullptr_t) noexcept:handle(nullptr) {}

	CurlMulti(CurlMulti &&src) noexcept
		:handle(std::exchange(src.handle, nullptr)) {}

	~CurlMulti() noexcept {
		if (handle != nullptr)
			curl_multi_cleanup(handle);
	}

	CurlMulti &operator=(CurlMulti &&src) noexcept {
		std::swap(handle, src.handle);
		return *this;
	}

	operator bool() const noexcept {
		return handle != nullptr;
	}

	CURLM *Get() noexcept {
		return handle;
	}

	template<typename T>
	void SetOption(CURLMoption option, T value) {
		auto code = curl_multi_setopt(handle, option, value);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
	}

	void Add(CURL *easy) {
		auto code = curl_multi_add_handle(handle, easy);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
	}

	void Remove(CURL *easy) {
		auto code = curl_multi_remove_handle(handle, easy);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
	}

	CURLMsg *InfoRead() noexcept {
		int msgs_in_queue;
		return curl_multi_info_read(handle, &msgs_in_queue);
	}

	unsigned Perform() {
		int running_handles;
		auto code = curl_multi_perform(handle, &running_handles);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
		return running_handles;
	}

	unsigned Wait(int timeout=-1) {
		int numfds;
		auto code = curl_multi_wait(handle, nullptr, 0, timeout,
					    &numfds);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
		return numfds;
	}

	unsigned Wait(std::chrono::milliseconds timeout) {
		return Wait(timeout.count());
	}
};
