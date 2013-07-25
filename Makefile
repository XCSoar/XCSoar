#
# This is the XCSoar build script.  To compile XCSoar, you must
# specify the target platform, e.g. for Pocket PC 2003, type:
#
#   make TARGET=PPC2003
#
# The following parameters may be specified on the "make" command
# line:
#
#   TARGET      The name of the target platform.  See the TARGETS variable
#               in build/targets.mk for a list of valid target platforms.
#
#   HEADLESS    If set to "y", no UI is available.
#
#   VFB         "y" means software rendering to non-interactive virtual
#               frame buffer
#
#   USE_FB      "y" means software rendering to /dev/fb0
#
#   ENABLE_SDL  If set to "y", the UI is drawn with libSDL.
#
#   OPENGL      "y" means render with OpenGL.
#
#   GLES        "y" means render with OpenGL/ES.
#
#   GREYSCALE   "y" means render 8-bit greyscale internally
#
#   DITHER      "y" means dither to 1-bit black&white
#
#   EYE_CANDY   "n" disables eye candy rendering.
#
#   DEBUG       If set to "y", the debugging version of XCSoar is built
#               (default is "y")
#
#   WERROR      Make all compiler warnings fatal (default is $DEBUG)
#
#   V           Verbosity; 1 is the default, and prints terse information.
#               0 means quiet, and 2 prints the full compiler commands.
#
#   FIXED       "y" means use fixed point math (for FPU-less platforms)
#
#   LTO         "y" enables gcc's link-time optimization flag (experimental,
#               requires gcc 4.5)
#
#   CLANG       "y" to use clang instead of gcc
#
#   ANALYZER    "y" to support the clang analyzer
#
#   LLVM        "y" to compile LLVM bitcode with clang
#
#   LIBCXX      The absolute path of the libc++ svn/git working directory.
#

.DEFAULT_GOAL := all

topdir = .

-include $(topdir)/build/local-config.mk

include $(topdir)/build/make.mk
include $(topdir)/build/bool.mk
include $(topdir)/build/string.mk
include $(topdir)/build/dirs.mk
include $(topdir)/build/verbose.mk
include $(topdir)/build/util.mk
include $(topdir)/build/detect.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/pkgconfig.mk
include $(topdir)/build/resource.mk
include $(topdir)/build/options.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/coverage.mk
include $(topdir)/build/libintl.mk

ifeq ($(HEADLESS),y)
else
include $(topdir)/build/vfb.mk
include $(topdir)/build/fb.mk
include $(topdir)/build/egl.mk
include $(topdir)/build/opengl.mk
include $(topdir)/build/sdl.mk
endif

include $(topdir)/build/flags.mk
include $(topdir)/build/charset.mk
include $(topdir)/build/warnings.mk
include $(topdir)/build/compile.mk
include $(topdir)/build/link.mk
include $(topdir)/build/java.mk
include $(topdir)/build/android.mk
include $(topdir)/build/llvm.mk
include $(topdir)/build/tools.mk
include $(topdir)/build/version.mk
include $(topdir)/build/osx.mk
include $(topdir)/build/generate.mk
include $(topdir)/build/doxygen.mk
include $(topdir)/build/manual.mk

# Create libraries for zzip, jasper and compatibility stuff
include $(topdir)/build/libstdcxx.mk
include $(topdir)/build/libutil.mk
include $(topdir)/build/libmath.mk
include $(topdir)/build/libgeo.mk
include $(topdir)/build/libos.mk
include $(topdir)/build/libtime.mk
include $(topdir)/build/libprofile.mk
include $(topdir)/build/libnet.mk
include $(topdir)/build/zlib.mk
include $(topdir)/build/zzip.mk
include $(topdir)/build/jasper.mk
include $(topdir)/build/libport.mk
include $(topdir)/build/driver.mk
include $(topdir)/build/libio.mk
include $(topdir)/build/libasync.mk
include $(topdir)/build/shapelib.mk
include $(topdir)/build/libwaypoint.mk
include $(topdir)/build/libairspace.mk
include $(topdir)/build/libtask.mk
include $(topdir)/build/libroute.mk
include $(topdir)/build/libcontest.mk
include $(topdir)/build/libglide.mk
include $(topdir)/build/datafield.mk
include $(topdir)/build/libevent.mk
include $(topdir)/build/freetype.mk
include $(topdir)/build/libpng.mk
include $(topdir)/build/libjpeg.mk
include $(topdir)/build/screen.mk
include $(topdir)/build/libthread.mk
include $(topdir)/build/form.mk
include $(topdir)/build/libwidget.mk
include $(topdir)/build/libaudio.mk
include $(topdir)/build/libterrain.mk
include $(topdir)/build/harness.mk

include $(topdir)/build/setup.mk
include $(topdir)/build/launch.mk
include $(topdir)/build/vali.mk
include $(topdir)/build/main.mk
include $(topdir)/build/kobo.mk
include $(topdir)/build/test.mk
include $(topdir)/build/hot.mk

# Load local-config a second time
# to set (override) choices for GXX and friends.
-include $(topdir)/build/local-config.mk

######## output files

include $(topdir)/build/dist.mk
include $(topdir)/build/install.mk

######## compiler flags

INCLUDES += -I$(SRC) -I$(ENGINE_SRC_DIR)

####### sources

include $(topdir)/build/gettext.mk
include $(topdir)/build/cab.mk

OUTPUTS := $(XCSOAR_BIN) $(VALI_XCS_BIN)
OUTPUTS += $(XCSOARSETUP_DLL) $(XCSOARLAUNCH_DLL)

ifeq ($(TARGET),ANDROID)
OUTPUTS += $(ANDROID_BIN)/XCSoar-debug.apk
endif

ifeq ($(HAVE_WIN32),y)
OUTPUTS += $(LAUNCH_XCSOAR_BIN)
endif

all: $(OUTPUTS)
everything: $(OUTPUTS) $(OPTIONAL_OUTPUTS) debug build-check build-harness

clean: FORCE
	@$(NQ)echo "cleaning all"
	$(Q)rm -rf build/local-config.mk
	$(Q)rm -rf $(OUT)
	$(RM) $(BUILDTESTS)

.PHONY: FORCE

ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/src/*/*/*/*/*/*.d)
endif
ifneq ($(wildcard $(TARGET_OUTPUT_DIR)/test/src/*.d),)
include $(wildcard $(TARGET_OUTPUT_DIR)/test/src/*.d)
endif
