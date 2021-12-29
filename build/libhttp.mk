# Build rules for the HTTP client library

HAVE_HTTP := y

LIBHTTP_SOURCES = \
	$(SRC)/net/http/DownloadManager.cpp \
	$(SRC)/net/http/Progress.cpp \
	$(SRC)/net/http/OutputStreamHandler.cxx \
	$(SRC)/net/http/FormData.cpp \
	$(SRC)/net/http/Adapter.cxx \
	$(SRC)/net/http/Setup.cxx \
	$(SRC)/net/http/Request.cxx \
	$(SRC)/net/http/CoRequest.cxx \
	$(SRC)/net/http/CoStreamRequest.cxx \
	$(SRC)/net/http/CoDownloadToFile.cpp \
	$(SRC)/net/http/Global.cxx \
	$(SRC)/net/http/Init.cpp

ifeq ($(TARGET_IS_OSX),y)
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
LIBHTTP_LDADD = $(ZLIB_LDADD)
LIBHTTP_LDLIBS = $(CURL_LDLIBS) $(ZLIB_LDLIBS)
endif

LIBHTTP_LDLIBS += $(LIBSODIUM_LDLIBS)

$(eval $(call link-library,libhttp,LIBHTTP))
