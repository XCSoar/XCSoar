# Build rules for the generic utility library

UTIL_SRC_DIR = $(SRC)/util

UTIL_SOURCES = \
	$(UTIL_SRC_DIR)/DecimalParser.cxx \
	$(UTIL_SRC_DIR)/Exception.cxx \
	$(UTIL_SRC_DIR)/PrintException.cxx \
	$(UTIL_SRC_DIR)/CRC16CCITT.cpp \
	$(UTIL_SRC_DIR)/UTF8.cpp \
	$(UTIL_SRC_DIR)/ASCII.cxx \
	$(UTIL_SRC_DIR)/TruncateString.cpp \
	$(UTIL_SRC_DIR)/EscapeBackslash.cpp \
	$(UTIL_SRC_DIR)/ConvertString.cpp \
	$(UTIL_SRC_DIR)/StaticString.cxx \
	$(UTIL_SRC_DIR)/StringBuilder.cxx \
	$(UTIL_SRC_DIR)/StringCompare.cxx \
	$(UTIL_SRC_DIR)/StringStrip.cxx \
	$(UTIL_SRC_DIR)/StringUtil.cpp

$(eval $(call link-library,util,UTIL))
