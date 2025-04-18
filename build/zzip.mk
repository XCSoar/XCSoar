ZZIPSRC	= $(SRC)/zzip
ZZIP_SOURCES = \
	$(ZZIPSRC)/fetch.c \
	$(ZZIPSRC)/file.c 		\
	$(ZZIPSRC)/plugin.c \
	$(ZZIPSRC)/zip.c \
	$(ZZIPSRC)/stat.c

ZZIP_CFLAGS = -Wno-strict-aliasing

ZZIP_DEPENDS = ZLIB

$(eval $(call link-library,zzip,ZZIP))
