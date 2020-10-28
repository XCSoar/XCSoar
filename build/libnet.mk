# Build rules for the HTTP client library

LIBNET_SOURCES = \
	$(SRC)/net/State.cpp \
	$(SRC)/net/IPv4Address.cxx \
	$(SRC)/net/IPv6Address.cxx \
	$(SRC)/net/StaticSocketAddress.cxx \
	$(SRC)/net/AllocatedSocketAddress.cxx \
	$(SRC)/net/SocketAddress.cxx \
	$(SRC)/net/SocketDescriptor.cxx

HAVE_HTTP := y

LIBNET_SOURCES += \
	$(SRC)/net/http/Multi.cpp \
	$(SRC)/net/http/Session.cpp \
	$(SRC)/net/http/Request.cpp \
	$(SRC)/net/http/FormData.cpp \
	$(SRC)/net/http/Init.cpp

ifeq ($(TARGET_IS_OSX),y)
# We use the libcurl which is included in Mac OS X.
# Mac OS X SDKs contain the required headers / library stubs,
# but no pkg-config file.
LIBNET_LDLIBS = -lcurl
else
$(eval $(call pkg-config-library,CURL,libcurl))

LIBNET_CPPFLAGS = $(CURL_CPPFLAGS)
LIBNET_LDADD = $(ZLIB_LDADD)
LIBNET_LDLIBS = $(CURL_LDLIBS) $(ZLIB_LDLIBS)
endif

LIBNET_LDLIBS += $(LIBSODIUM_LDLIBS)

ifeq ($(HAVE_HTTP),y)

LIBNET_SOURCES += \
	$(SRC)/net/http/DownloadManager.cpp \
	$(SRC)/net/http/ToFile.cpp \
	$(SRC)/net/http/ToBuffer.cpp

endif

$(eval $(call link-library,libnet,LIBNET))
