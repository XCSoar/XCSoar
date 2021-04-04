/*
 * Copyright 2008-2018 Max Kellermann <max.kellermann@gmail.com>
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

#ifndef CURL_REQUEST_HXX
#define CURL_REQUEST_HXX

#include "Easy.hxx"

#include <map>
#include <string>

struct StringView;
class CurlGlobal;
class CurlResponseHandler;

class CurlRequest final {
	CurlGlobal &global;

	CurlResponseHandler &handler;

	/** the curl handle */
	CurlEasy easy;

	enum class State {
		HEADERS,
		BODY,
		CLOSED,
	} state = State::HEADERS;

	std::multimap<std::string, std::string> headers;

	/** error message provided by libcurl */
	char error_buffer[CURL_ERROR_SIZE];

	bool registered = false;

public:
	/**
	 * To start sending the request, call Start().
	 */
	CurlRequest(CurlGlobal &_global,
		    CurlResponseHandler &_handler);

	CurlRequest(CurlGlobal &_global, const char *url,
		    CurlResponseHandler &_handler)
		:CurlRequest(_global, _handler) {
		SetUrl(url);
	}

	~CurlRequest() noexcept;

	CurlRequest(const CurlRequest &) = delete;
	CurlRequest &operator=(const CurlRequest &) = delete;

	/**
	 * Register this request via CurlGlobal::Add(), which starts
	 * the request.
	 *
	 * This method must be called in the event loop thread.
	 */
	void Start();

	/**
	 * A thread-safe version of Start().
	 */
	void StartIndirect();

	/**
	 * Unregister this request via CurlGlobal::Remove().
	 *
	 * This method must be called in the event loop thread.
	 */
	void Stop() noexcept;

	/**
	 * A thread-safe version of Stop().
	 */
	void StopIndirect();

	CURL *Get() noexcept {
		return easy.Get();
	}

	template<typename T>
	void SetOption(CURLoption option, T value) {
		easy.SetOption(option, value);
	}

	void SetUrl(const char *url) {
		easy.SetURL(url);
	}

	void SetRequestHeaders(struct curl_slist *request_headers) {
		easy.SetRequestHeaders(request_headers);
	}

	void SetFailOnError(bool value=true) {
		SetOption(CURLOPT_FAILONERROR, (long)value);
	}

	void SetVerifyHost(bool value) {
		easy.SetVerifyHost(value);
	}

	void SetVerifyPeer(bool value) {
		easy.SetVerifyPeer(value);
	}

	void SetNoBody(bool value=true) {
		easy.SetNoBody(value);
	}

	void SetPost(bool value=true) {
		easy.SetPost(value);
	}

	void SetRequestBody(const void *data, size_t size) {
		easy.SetRequestBody(data, size);
	}

	void Resume() noexcept;

	/**
	 * A HTTP request is finished.  Called by #CurlGlobal.
	 */
	void Done(CURLcode result) noexcept;

private:
	/**
	 * Frees the current "libcurl easy" handle, and everything
	 * associated with it.
	 */
	void FreeEasy() noexcept;

	void FinishHeaders();
	void FinishBody();

	size_t DataReceived(const void *ptr, size_t size) noexcept;

	void HeaderFunction(StringView s) noexcept;

	/** called by curl when new data is available */
	static size_t _HeaderFunction(char *ptr, size_t size, size_t nmemb,
				      void *stream) noexcept;

	/** called by curl when new data is available */
	static size_t WriteFunction(char *ptr, size_t size, size_t nmemb,
				    void *stream) noexcept;
};

#endif
