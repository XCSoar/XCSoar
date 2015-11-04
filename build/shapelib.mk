SHPSRC = $(SRC)/Topography/shapelib

SHAPELIB_SOURCES = \
	$(SHPSRC)/mapalloc.c \
	$(SHPSRC)/mapstring.c \
	$(SHPSRC)/mapbits.c 	\
	$(SHPSRC)/mapprimitive.c 	\
	$(SHPSRC)/mapsearch.c 	\
	$(SHPSRC)/mapshape.c 	\
	$(SHPSRC)/maptree.c 	\
	$(SHPSRC)/mapxbase.c 	

$(eval $(call link-library,shapelib,SHAPELIB))
