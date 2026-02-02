# ANGLE (Almost Native Graphics Layer Engine) integration
# Provides OpenGL ES implementation via Metal backend on macOS
#
# Set ANGLE_PREFIX to the ANGLE installation directory:
#   make ANGLE_PREFIX=/path/to/angle
# or via environment variable

ifeq ($(TARGET_IS_DARWIN),y)

USE_ANGLE ?= y

ifeq ($(USE_ANGLE),y)

# Default to output directory if not specified
ANGLE_PREFIX ?= $(OUT)/src/angle

# Auto-fetch ANGLE if not present
ANGLE_FETCH_SCRIPT = $(topdir)/darwin/fetch-angle-from-chrome.sh
ANGLE_FETCH_STAMP = $(ANGLE_PREFIX)/.stamp
ANGLE_FIX_STAMP = $(ANGLE_PREFIX)/.install_name_fixed

$(ANGLE_FETCH_STAMP):
	@$(NQ)echo "  FETCH   ANGLE libraries"
	$(Q)$(ANGLE_FETCH_SCRIPT) $(ANGLE_PREFIX) $(OUT)/angle-download && touch $@

# Fix install names so executables can use rpath lookup
$(ANGLE_FIX_STAMP): $(ANGLE_FETCH_STAMP)
	@$(NQ)echo "  FIX     ANGLE install names"
	$(Q)install_name_tool -id @rpath/libEGL.dylib $(ANGLE_PREFIX)/lib/libEGL.dylib
	$(Q)install_name_tool -id @rpath/libGLESv2.dylib $(ANGLE_PREFIX)/lib/libGLESv2.dylib
	$(Q)touch $@

# Add ANGLE fetch to compile dependencies
compile-depends += $(ANGLE_FIX_STAMP)

# Add ANGLE to compile dependencies
# Include and library paths (standard layout with include/ and lib/ subdirectories)
ANGLE_PREFIX_ABS := $(abspath $(ANGLE_PREFIX))
ANGLE_CPPFLAGS := -I$(ANGLE_PREFIX_ABS)/include -DUSE_ANGLE
ANGLE_LDLIBS := -L$(ANGLE_PREFIX_ABS)/lib

# ANGLE libraries
ANGLE_LDLIBS += -lEGL -lGLESv2

# Add rpath so binary can find ANGLE libraries at runtime
ANGLE_LDLIBS += -Wl,-rpath,$(ANGLE_PREFIX_ABS)/lib

# System frameworks required by ANGLE's Metal backend
ANGLE_LDLIBS += -framework Metal -framework CoreFoundation -framework IOSurface -framework CoreGraphics

endif

endif
