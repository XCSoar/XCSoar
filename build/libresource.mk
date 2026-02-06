# Build rules for the resource library

ifeq ($(TARGET_IS_ANDROID),n)

RESOURCE_SOURCES := \
	$(SRC)/ResourceLoader.cpp \
	$(SRC)/ResourceLookup.cpp

$(eval $(call link-library,libresource,RESOURCE))

RESOURCE_LDADD += $(RESOURCE_BINARY)

else

# On Android, ResourceLoader is not used (assets are loaded via
# Java), but ResourceLookup is still needed so that RichTextWindow
# can resolve "resource:IDB_NAME" image URLs.
RESOURCE_SOURCES := \
	$(SRC)/ResourceLookup.cpp

$(eval $(call link-library,libresource,RESOURCE))

endif
