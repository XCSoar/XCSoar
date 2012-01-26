# Build rules for the generic utility library

UTIL_SRC_DIR = $(SRC)/Util
INCLUDES += -I$(UTIL_SRC_DIR)

UTIL_SOURCES = \
	$(UTIL_SRC_DIR)/tstring.cpp \
	$(UTIL_SRC_DIR)/UTF8.cpp \
	$(UTIL_SRC_DIR)/EscapeBackslash.cpp \
	$(UTIL_SRC_DIR)/StringUtil.cpp

$(eval $(call link-library,util,UTIL))
