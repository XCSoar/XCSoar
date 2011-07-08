# Build rules for the HTTP client library

LIBNET_SOURCES =
HAVE_NET := n

ifneq ($(findstring $(TARGET),PC WINE),)
HAVE_NET := y
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
