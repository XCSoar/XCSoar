JASSRC = $(SRC)/jasper
JASPER = \
	$(JASSRC)/base/jas_debug.c \
	$(JASSRC)/base/jas_malloc.c \
	$(JASSRC)/base/jas_seq.c 	$(JASSRC)/base/jas_stream.c \
	$(JASSRC)/base/jas_string.c 	$(JASSRC)/base/jas_tvp.c \
	$(JASSRC)/jp2/jp2_cod.c \
	$(JASSRC)/jp2/jp2_dec.c 	$(JASSRC)/jpc/jpc_bs.c \
	$(JASSRC)/jpc/jpc_cs.c 		$(JASSRC)/jpc/jpc_dec.c \
	$(JASSRC)/jpc/jpc_math.c 	$(JASSRC)/jpc/jpc_mct.c \
	$(JASSRC)/jpc/jpc_mqdec.c       $(JASSRC)/jpc/jpc_mqcod.c \
	$(JASSRC)/jpc/jpc_qmfb.c 	$(JASSRC)/jpc/jpc_rtc.cpp \
	$(JASSRC)/jpc/jpc_t1dec.c \
	$(JASSRC)/jpc/jpc_t1cod.c \
	$(JASSRC)/jpc/jpc_t2dec.c 	$(JASSRC)/jpc/jpc_t2cod.c \
	$(JASSRC)/jpc/jpc_tagtree.c	$(JASSRC)/jpc/jpc_tsfb.c \
	$(JASSRC)/jpc/jpc_util.c

ifneq ($(CLANG),y)
ifneq ($(TARGET),CYGWIN)
$(call SRC_TO_OBJ,$(JASPER)): CFLAGS += -Wno-unused-but-set-parameter -Wno-unused-but-set-variable
endif
endif

JASPER_LIBS = $(TARGET_OUTPUT_DIR)/jasper.a

$(JASPER_LIBS): CFLAGS += -Wno-type-limits
$(JASPER_LIBS): $(call SRC_TO_OBJ,$(JASPER))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
