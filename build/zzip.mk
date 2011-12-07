ZZIPSRC	= $(SRC)/zzip
ZZIP_SOURCES = \
	$(ZZIPSRC)/fetch.c \
	$(ZZIPSRC)/file.c 		\
	$(ZZIPSRC)/plugin.c \
	$(ZZIPSRC)/zip.c \
	$(ZZIPSRC)/stat.c

ZZIP_CFLAGS = -Wno-strict-aliasing
ZZIP_CPPFLAGS_INTERNAL = $(ZLIB_CPPFLAGS)

$(eval $(call link-library,zzip,ZZIP))

ZZIP_LDADD += $(ZLIB_LDADD)
ZZIP_LDLIBS += $(ZLIB_LDLIBS)
