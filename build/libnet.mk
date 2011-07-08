# Build rules for the HTTP client library

LIBNET_SOURCES =

ifeq ($(HAVE_WIN32),y)
LIBNET_SOURCES += \
	$(SRC)/Net/WinINet/Session.cpp \
	$(SRC)/Net/WinINet/Connection.cpp \
	$(SRC)/Net/WinINet/Request.cpp
endif

LIBNET_OBJS = $(call SRC_TO_OBJ,$(LIBNET_SOURCES))
LIBNET_LIBS = $(TARGET_OUTPUT_DIR)/libnet.a

$(LIBNET_LIBS): $(LIBNET_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
