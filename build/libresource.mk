# Build rules for the resource library

RESOURCE_SOURCES := \
	$(SRC)/ResourceLoader.cpp

$(eval $(call link-library,libresource,RESOURCE))

RESOURCE_LDADD += $(RESOURCE_BINARY)
