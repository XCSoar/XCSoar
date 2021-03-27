/*
 * Copyright 2011-2021 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CURL_MULTI_HXX
#define CURL_MULTI_HXX

#include <curl/curl.h>

#include <map>
#include <stdexcept>

/**
 * An OO wrapper for a "CURLM*" (a libCURL "multi" handle).
 */
class CurlMulti {
	CURLM *handle = nullptr;

	std::map<const CURL *, CURLcode> results;

public:
	CurlMulti();
	CurlMulti(const CurlMulti &other) = delete;

	~CurlMulti();

	CurlMulti &operator=(const CurlMulti &other) = delete;

	bool IsDefined() const {
		return handle != nullptr;
	}

	void Add(CURL *easy) {
		CURLMcode code = curl_multi_add_handle(handle, easy);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
	}

	void Remove(CURL *easy);

	void FdSet(fd_set *read_fd_set, fd_set *write_fd_set, fd_set *exc_fd_set,
		   int *max_fd) const {
		CURLMcode code = curl_multi_fdset(handle, read_fd_set, write_fd_set,
						  exc_fd_set, max_fd);
		if (code != CURLM_OK)
			throw std::runtime_error(curl_multi_strerror(code));
	}

	[[gnu::pure]]
	long GetTimeout() const {
		long timeout;
		return ::curl_multi_timeout(handle, &timeout) == CURLM_OK
			? timeout
			: -1;
	}

	CURLMcode Perform() {
		int running_handles;
		return ::curl_multi_perform(handle, &running_handles);
	}

	[[gnu::pure]]
	CURLcode InfoRead(const CURL *easy);
};

#endif
