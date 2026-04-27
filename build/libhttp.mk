ifeq ($(HAVE_HTTP),y)
# Build rules for the HTTP client library

LIBHTTP_SOURCES = \
	$(SRC)/net/http/DownloadManager.cpp \
	$(SRC)/net/http/Progress.cpp \
	$(SRC)/lib/curl/OutputStreamHandler.cxx \
	$(SRC)/lib/curl/Adapter.cxx \
	$(SRC)/lib/curl/Setup.cxx \
	$(SRC)/lib/curl/Request.cxx \
	$(SRC)/lib/curl/CoRequest.cxx \
	$(SRC)/lib/curl/CoStreamRequest.cxx \
	$(SRC)/net/http/CoDownloadToFile.cpp \
	$(SRC)/lib/curl/Global.cxx \
	$(SRC)/net/http/Init.cpp

ifeq ($(TARGET_IS_OSX)$(USE_HOMEBREW),yn)
# On macOS without Homebrew/pkg-config, link libcurl manually.
# Depending on the active search paths this may resolve to either:
# - the curl stubs from the macOS SDK/system, or
# - a third-party static libcurl.
# In both cases we provide the required Apple frameworks explicitly for
# SecureTransport/Security symbols used by curl.
LIBHTTP_LDLIBS = -lcurl -lz -framework Security -framework CoreFoundation -framework CoreServices
else
$(eval $(call pkg-config-library,CURL,libcurl))

ifeq ($(USE_THIRDPARTY_LIBS),y)
# This definition is missing in the CURL cmake build
CURL_CPPFLAGS += -DCURL_STATICLIB
endif

LIBHTTP_CPPFLAGS += $(CURL_CPPFLAGS)
LIBHTTP_DEPENDS += CURL ZLIB
endif

LIBHTTP_DEPENDS += LIBSODIUM FMT UTIL

$(eval $(call link-library,libhttp,LIBHTTP))
endif
