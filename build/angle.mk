# ANGLE (Almost Native Graphics Layer Engine) integration
# Provides OpenGL ES implementation via Metal backend on macOS,
# or via D3D11 backend on Windows.
#
# Set ANGLE_PREFIX to the ANGLE installation directory:
#   make ANGLE_PREFIX=/path/to/angle
# or via environment variable

# Enable ANGLE support for Darwin or Windows (when USE_ANGLE=y)
ifeq ($(TARGET_IS_DARWIN),y)
USE_ANGLE ?= y
else ifeq ($(HAVE_WIN32),y)
USE_ANGLE ?= n
endif

ifeq ($(USE_ANGLE),y)

# Default to output directory if not specified
ANGLE_PREFIX ?= $(OUT)/src/angle

# Auto-fetch ANGLE if not present
ifeq ($(TARGET_IS_DARWIN),y)
ANGLE_FETCH_SCRIPT = $(topdir)/darwin/fetch-angle-from-chrome.sh
else ifeq ($(HAVE_WIN32),y)
ANGLE_FETCH_SCRIPT = $(topdir)/windows/fetch-angle-from-chrome.sh
endif

ANGLE_FETCH_STAMP = $(ANGLE_PREFIX)/.stamp
ANGLE_FIX_STAMP = $(ANGLE_PREFIX)/.install_name_fixed

$(ANGLE_FETCH_STAMP):
	@$(NQ)echo "  FETCH   ANGLE libraries"
	$(Q)$(ANGLE_FETCH_SCRIPT) $(ANGLE_PREFIX) $(OUT)/angle-download && touch $@

ifeq ($(TARGET_IS_DARWIN),y)
# Fix install names so executables can use rpath lookup
$(ANGLE_FIX_STAMP): $(ANGLE_FETCH_STAMP)
	@$(NQ)echo "  FIX     ANGLE install names"
	$(Q)install_name_tool -id @rpath/libEGL.dylib $(ANGLE_PREFIX)/lib/libEGL.dylib
	$(Q)install_name_tool -id @rpath/libGLESv2.dylib $(ANGLE_PREFIX)/lib/libGLESv2.dylib
	$(Q)touch $@

# Add ANGLE fetch+fix to compile dependencies
compile-depends += $(ANGLE_FIX_STAMP)

else ifeq ($(HAVE_WIN32),y)
# Add ANGLE fetch to compile dependencies
compile-depends += $(ANGLE_FETCH_STAMP)
endif

# Add ANGLE to compile dependencies
# Include and library paths (standard layout with include/ and lib/ subdirectories)
ANGLE_PREFIX_ABS := $(abspath $(ANGLE_PREFIX))
ANGLE_CPPFLAGS := -I$(ANGLE_PREFIX_ABS)/include -DUSE_ANGLE
ANGLE_LDLIBS := -L$(ANGLE_PREFIX_ABS)/lib

# ANGLE libraries
ANGLE_LDLIBS += -lEGL -lGLESv2

ifeq ($(TARGET_IS_DARWIN),y)
# Add rpath so binary can find ANGLE libraries at runtime
ANGLE_LDLIBS += -Wl,-rpath,$(ANGLE_PREFIX_ABS)/lib

# System frameworks required by ANGLE's Metal backend
ANGLE_LDLIBS += -framework Metal -framework CoreFoundation -framework IOSurface -framework CoreGraphics
endif

ifeq ($(HAVE_WIN32),y)
# Windows: D3D11 backend, libraries are linked via import libs (.dll.a)
# No special system libraries needed - D3D11 is loaded dynamically by ANGLE

# Copy ANGLE DLLs to bin directory for runtime
ANGLE_BIN_DLLS = $(TARGET_BIN_DIR)/libEGL.dll $(TARGET_BIN_DIR)/libGLESv2.dll

$(TARGET_BIN_DIR)/libEGL.dll: $(ANGLE_FETCH_STAMP) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  COPY    $(@F)"
	$(Q)cp $(ANGLE_PREFIX_ABS)/bin/libEGL.dll $@

$(TARGET_BIN_DIR)/libGLESv2.dll: $(ANGLE_FETCH_STAMP) | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  COPY    $(@F)"
	$(Q)cp $(ANGLE_PREFIX_ABS)/bin/libGLESv2.dll $@

# Add ANGLE DLLs to compile dependencies so they get copied to bin directory
compile-depends += $(ANGLE_BIN_DLLS)

endif

endif
