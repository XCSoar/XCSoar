# Rules for VALI-XCS.exe, the non-interactive G record validation tool

VALI_XCS = $(TARGET_BIN_DIR)/vali-xcs$(TARGET_EXEEXT)
VALI_XCS_NS = $(TARGET_BIN_DIR)/vali-xcs$(NOSTRIP_SUFFIX)$(TARGET_EXEEXT)

VALI_XCS_SOURCES = \
	$(SRC)/Logger/LoggerGRecord.cpp \
	$(SRC)/Logger/MD5.cpp \
	$(SRC)/Util/UTF8.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/VALI-XCS.cpp
VALI_XCS_DEPENDS = IO
VALI_XCS_STRIP = y

$(eval $(call link-program,vali-xcs,VALI_XCS))
