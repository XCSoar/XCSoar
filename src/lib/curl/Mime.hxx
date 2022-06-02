/*
 * Copyright 2021 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef CURL_MIME_HXX
#define CURL_MIME_HXX

#include <curl/curl.h>

#include <utility>

/**
 * An OO wrapper for a "curl_mime" object.
 */
class CurlMime {
	curl_mime *mime;

public:
	explicit CurlMime(CURL *easy) noexcept
		:mime(curl_mime_init(easy)) {}

	CurlMime(CurlMime &&src) noexcept
		:mime(std::exchange(src.mime, nullptr)) {}

	~CurlMime() noexcept {
		if (mime != nullptr)
			curl_mime_free(mime);
	}

	auto &operator=(CurlMime &&src) noexcept {
		using std::swap;
		swap(mime, src.mime);
		return *this;
	}

	curl_mime *get() const noexcept {
		return mime;
	}

	class Part {
		curl_mimepart *part;

	public:
		constexpr Part(curl_mimepart *_part) noexcept
			:part(_part) {}

		auto Name(const char *name) noexcept {
			curl_mime_name(part, name);
			return *this;
		}

		auto Data(const void *data, std::size_t size) noexcept {
			curl_mime_data(part, (const char *)data, size);
			return *this;
		}

		auto Data(const char *data) noexcept {
			curl_mime_data(part, data, CURL_ZERO_TERMINATED);
			return *this;
		}

		auto FileData(const char *path) noexcept {
			curl_mime_filedata(part, path);
			return *this;
		}

		auto Filename(const char *name) noexcept {
			curl_mime_filename(part, name);
			return *this;
		}

		auto Type(const char *type) noexcept {
			curl_mime_type(part, type);
			return *this;
		}
	};

	Part Add() const noexcept {
		return curl_mime_addpart(mime);
	}

	Part Add(const char *name) const noexcept {
		return Add().Name(name);
	}
};

#endif
