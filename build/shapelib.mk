SHPSRC = $(SRC)/Screen/shapelib

SHAPELIB = \
	$(SHPSRC)/mapbits.c 	\
	$(SHPSRC)/maperror.c 	\
	$(SHPSRC)/mapprimitive.c 	\
	$(SHPSRC)/mapsearch.c 	\
	$(SHPSRC)/mapshape.c 	\
	$(SHPSRC)/maptree.c 	\
	$(SHPSRC)/mapxbase.c 	

SHAPELIB_LIBS = $(SRC)/shapelib-$(TARGET).a

$(SHAPELIB_LIBS): $(call SRC_TO_OBJ,$(SHAPELIB))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
