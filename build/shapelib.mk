SHPSRC = $(SRC)/Topography/shapelib

SHAPELIB = \
	$(SHPSRC)/mapstring.c \
	$(SHPSRC)/mapbits.c 	\
	$(SHPSRC)/mapfile.c \
	$(SHPSRC)/mapprimitive.c 	\
	$(SHPSRC)/mapsearch.c 	\
	$(SHPSRC)/mapshape.c 	\
	$(SHPSRC)/maptree.c 	\
	$(SHPSRC)/mapxbase.c 	

SHAPELIB_LIBS = $(TARGET_OUTPUT_DIR)/shapelib.a

$(SHAPELIB_LIBS): $(call SRC_TO_OBJ,$(SHAPELIB))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
