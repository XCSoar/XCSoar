SHPSRC = $(SRC)/Screen/shapelib

SHAPELIB = \
	$(SHPSRC)/mapbits.c 	\
	$(SHPSRC)/maperror.c 	\
	$(SHPSRC)/mapprimitive.c 	\
	$(SHPSRC)/mapsearch.c 	\
	$(SHPSRC)/mapshape.c 	\
	$(SHPSRC)/maptree.c 	\
	$(SHPSRC)/mapxbase.c 	

$(SRC)/shapelib-$(TARGET).a: $(call SRC_TO_OBJ,$(SHAPELIB))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
