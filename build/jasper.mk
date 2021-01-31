JASSRC = $(SRC)/Terrain/jasper
JASPER_SOURCES = \
	$(JASSRC)/base/jas_malloc.c \
	$(JASSRC)/base/jas_seq.c 	$(JASSRC)/base/jas_stream.c \
	$(JASSRC)/jp2/jp2_cod.c \
	$(JASSRC)/jpc/jpc_bs.c \
	$(JASSRC)/jpc/jpc_cs.c 		$(JASSRC)/jpc/jpc_dec.c \
	$(JASSRC)/jpc/jpc_math.c \
	$(JASSRC)/jpc/jpc_mqdec.c       $(JASSRC)/jpc/jpc_mqcod.c \
	$(JASSRC)/jpc/jpc_qmfb.c 	$(JASSRC)/jpc/jpc_rtc.cpp \
	$(JASSRC)/jpc/jpc_t1dec.c \
	$(JASSRC)/jpc/jpc_t1cod.c \
	$(JASSRC)/jpc/jpc_t2dec.c 	$(JASSRC)/jpc/jpc_t2cod.c \
	$(JASSRC)/jpc/jpc_tagtree.c	$(JASSRC)/jpc/jpc_tsfb.c

JASPER_CFLAGS_INTERNAL = -Wno-type-limits
JASPER_CFLAGS_INTERNAL += -Wno-shift-negative-value -Wno-sign-compare -Wno-tautological-compare

ifneq ($(CLANG),y)
JASPER_CFLAGS_INTERNAL += -Wno-unused-but-set-parameter -Wno-unused-but-set-variable
endif

JASPER_CPPFLAGS = -I$(JASSRC)/..

$(eval $(call link-library,jasper,JASPER))
