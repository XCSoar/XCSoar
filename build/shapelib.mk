SHPSRC = $(SRC)/Screen/shapelib

SHAPELIB = \
	$(SHPSRC)/mapbits.c 	\
	$(SHPSRC)/maperror.c 	\
	$(SHPSRC)/mapprimitive.c 	\
	$(SHPSRC)/mapsearch.c 	\
	$(SHPSRC)/mapshape.c 	\
	$(SHPSRC)/maptree.c 	\
	$(SHPSRC)/mapxbase.c 	

$(SRC)/shapelib-$(TARGET).a: $(patsubst %.cpp,%-$(TARGET).o,$(SHAPELIB:.c=-$(TARGET).o))
	@$(NQ)echo "  AR      $@"
	$(Q)$(AR) $(ARFLAGS) $@ $^
