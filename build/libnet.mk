# Build rules for the HTTP client library

LIBNET_SOURCES =
HAVE_NET := n

ifneq ($(findstring $(TARGET),PC WINE CYGWIN),)
HAVE_NET := y
LIBNET_SOURCES += \
	$(SRC)/Net/WinINet/Session.cpp \
	$(SRC)/Net/WinINet/Connection.cpp \
	$(SRC)/Net/WinINet/Request.cpp
LIBNET_LDLIBS = -lwininet
endif

# don't link with CURL on Mac OS X, to keep the dynamic library
# dependencies low
ifeq ($(TARGET)$(findstring $(shell uname -s),Darwin),UNIX)
HAVE_NET := y

LIBNET_SOURCES += \
	$(SRC)/Net/CURL/Session.cpp \
	$(SRC)/Net/CURL/Request.cpp \
	$(SRC)/Net/CURL/Init.cpp

CURL_CPPFLAGS := $(shell $(PKG_CONFIG) libcurl --cflags)
CURL_LDLIBS := $(shell $(PKG_CONFIG) libcurl --libs)

LIBNET_CPPFLAGS = $(CURL_CPPFLAGS)
LIBNET_LDLIBS = $(CURL_LDLIBS)
endif

ifeq ($(TARGET),ANDROID)
HAVE_NET := y

LIBNET_SOURCES += \
	$(SRC)/Net/Java/Session.cpp \
	$(SRC)/Net/Java/Request.cpp
endif

ifeq ($(HAVE_NET),y)

LIBNET_OBJS = $(call SRC_TO_OBJ,$(LIBNET_SOURCES))
LIBNET_LIBS = $(TARGET_OUTPUT_DIR)/libnet.a

$(LIBNET_LIBS): $(LIBNET_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^

endif
