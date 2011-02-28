# Build rules for the generic utility library

UTIL_SRC_DIR = $(SRC)/Util
INCLUDES += -I$(UTIL_SRC_DIR)

UTIL_SOURCES = \
	$(UTIL_SRC_DIR)/tstring.cpp \
	$(UTIL_SRC_DIR)/UTF8.cpp \
	$(UTIL_SRC_DIR)/StringUtil.cpp

UTIL_OBJS = $(call SRC_TO_OBJ,$(UTIL_SOURCES))
UTIL_LIBS = $(TARGET_OUTPUT_DIR)/util.a

$(UTIL_LIBS): $(UTIL_OBJS)
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
