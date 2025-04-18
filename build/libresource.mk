# Build rules for the resource library

ifeq ($(TARGET_IS_ANDROID),n)

RESOURCE_SOURCES := \
	$(SRC)/ResourceLoader.cpp

$(eval $(call link-library,libresource,RESOURCE))

RESOURCE_LDADD += $(RESOURCE_BINARY)

else

RESOURCE_LDADD =

endif
