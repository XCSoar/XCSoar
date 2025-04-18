UNITS_SOURCES = \
	$(SRC)/Units/Descriptor.cpp \
	$(SRC)/Units/System.cpp

$(eval $(call link-library,libunits,UNITS))
