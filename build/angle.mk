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

ANGLE_FIX_STAMP =

ifeq ($(USE_ANGLE),y)

# Default to thirdparty output, with manual override fallback.
ANGLE_PREFIX ?= $(THIRDPARTY_LIBS_ROOT)
ifeq ($(ANGLE_PREFIX),)
ANGLE_PREFIX := $(OUT)/src/angle
endif

ANGLE_THIRDPARTY_STAMP := $(if $(THIRDPARTY_LIBS_DIR),$(THIRDPARTY_LIBS_DIR)/stamp)

# Include and library paths (standard layout with include/ and lib/ subdirectories)
ANGLE_PREFIX_ABS := $(abspath $(ANGLE_PREFIX))
ANGLE_CPPFLAGS := -I$(ANGLE_PREFIX_ABS)/include -DUSE_ANGLE
ANGLE_LDLIBS := -L$(ANGLE_PREFIX_ABS)/lib

# ANGLE libraries
ANGLE_LDLIBS += -lEGL -lGLESv2

ifeq ($(TARGET_IS_DARWIN),y)
# Fix install names so executables can use rpath lookup
ANGLE_FIX_STAMP = $(ANGLE_PREFIX_ABS)/.install_name_fixed

$(ANGLE_FIX_STAMP): $(ANGLE_THIRDPARTY_STAMP)
	@$(NQ)echo "  FIX     ANGLE install names"
	$(Q)install_name_tool -id @rpath/libEGL.dylib $(ANGLE_PREFIX_ABS)/lib/libEGL.dylib
	$(Q)install_name_tool -id @rpath/libGLESv2.dylib $(ANGLE_PREFIX_ABS)/lib/libGLESv2.dylib
	$(Q)touch $@

# Add ANGLE fix to compile dependencies
compile-depends += $(ANGLE_FIX_STAMP)

# Add rpath so binary can find ANGLE libraries at runtime
ANGLE_LDLIBS += -Wl,-rpath,$(ANGLE_PREFIX_ABS)/lib

# System frameworks required by ANGLE's Metal backend
ANGLE_LDLIBS += -framework Metal -framework CoreFoundation -framework IOSurface -framework CoreGraphics
endif

ifeq ($(HAVE_WIN32),y)
# Copy ANGLE runtime DLLs next to the executable.
ANGLE_BIN_DLLS = $(TARGET_BIN_DIR)/libEGL.dll $(TARGET_BIN_DIR)/libGLESv2.dll
ANGLE_SOURCE_DLLS = $(ANGLE_PREFIX_ABS)/bin/libEGL.dll $(ANGLE_PREFIX_ABS)/bin/libGLESv2.dll

ifneq ($(ANGLE_THIRDPARTY_STAMP),)
$(ANGLE_SOURCE_DLLS): $(ANGLE_THIRDPARTY_STAMP)
endif

$(TARGET_BIN_DIR)/libEGL.dll: $(ANGLE_PREFIX_ABS)/bin/libEGL.dll | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  COPY    $(@F)"
	$(Q)cp $< $@

$(TARGET_BIN_DIR)/libGLESv2.dll: $(ANGLE_PREFIX_ABS)/bin/libGLESv2.dll | $(TARGET_BIN_DIR)/dirstamp
	@$(NQ)echo "  COPY    $(@F)"
	$(Q)cp $< $@

compile-depends += $(ANGLE_BIN_DLLS)
endif

endif
