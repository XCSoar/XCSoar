# Build rules for the generic utility library

UTIL_SRC_DIR = $(SRC)/Util

UTIL_SOURCES = \
	$(UTIL_SRC_DIR)/CRC.cpp \
	$(UTIL_SRC_DIR)/tstring.cpp \
	$(UTIL_SRC_DIR)/UTF8.cpp \
	$(UTIL_SRC_DIR)/TruncateString.cpp \
	$(UTIL_SRC_DIR)/EscapeBackslash.cpp \
	$(UTIL_SRC_DIR)/ExtractParameters.cpp \
	$(UTIL_SRC_DIR)/ConvertString.cpp \
	$(UTIL_SRC_DIR)/StaticString.cxx \
	$(UTIL_SRC_DIR)/AllocatedString.cxx \
	$(UTIL_SRC_DIR)/StringUtil.cpp

ifeq ($(HAVE_MSVCRT),y)
UTIL_SOURCES += $(UTIL_SRC_DIR)/WStringUtil.cpp
endif

$(eval $(call link-library,util,UTIL))
