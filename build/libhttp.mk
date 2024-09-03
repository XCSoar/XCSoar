# Build rules for the HTTP client library

HAVE_HTTP := y

LIBHTTP_DEPENDS = LIBSODIUM FMT

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
# We use the libcurl which is included in Mac OS X.
# Mac OS X SDKs contain the required headers / library stubs,
# but no pkg-config file.
LIBHTTP_LDLIBS = -lcurl -lssl -lcrypto -lz
else
$(eval $(call pkg-config-library,CURL,libcurl))

ifeq ($(USE_THIRDPARTY_LIBS),y)
# This definition is missing in the CURL cmake build
CURL_CPPFLAGS += -DCURL_STATICLIB
endif

LIBHTTP_CPPFLAGS = $(CURL_CPPFLAGS)
LIBHTTP_DEPENDS += CURL ZLIB FMT
endif

$(eval $(call link-library,libhttp,LIBHTTP))
