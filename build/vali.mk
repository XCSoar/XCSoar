# Rules for VALI-XCS.exe, the non-interactive G record validation tool

VALI_XCS_SOURCES = \
	$(SRC)/Logger/GRecord.cpp \
	$(SRC)/util/MD5.cpp \
	$(SRC)/Version.cpp \
	$(SRC)/VALI-XCS.cpp
VALI_XCS_DEPENDS = IO OS UTIL
VALI_XCS_STRIP = y

$(eval $(call link-program,vali-xcs,VALI_XCS))
