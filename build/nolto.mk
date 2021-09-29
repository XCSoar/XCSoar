ifeq ($(LTO)$(CLANG),yn)

# These sources contain items like inline assembler which are not
# encoded in the intermediate code used by LTO, and therefore missing by the linker.
# therefore disable LTO for such sources
NOLTO_SOURCES = \
	$(DATA_SOURCES) \
	$(TARGET_OUTPUT_DIR)/resources.c

$(call SRC_TO_OBJ,$(NOLTO_SOURCES)): TARGET_OPTIMIZE += -fno-lto

endif
