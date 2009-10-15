#
# The XCSoar test suite.
#

topdir = ..

SRC = $(topdir)/Common/Source
HDR = $(topdir)/Common/Header

include $(topdir)/build/sdl.mk
include $(topdir)/build/targets.mk
include $(topdir)/build/debug.mk
include $(topdir)/build/verbose.mk
include $(topdir)/build/compile.mk

######## paths

INCLUDES := $(TARGET_INCLUDES) -I$(HDR) -I$(SRC)

######## compiler flags

CPPFLAGS := $(INCLUDES) $(TARGET_CPPFLAGS)
CPPFLAGS	+= -DFLARM_AVERAGE

CXXFLAGS	:=$(OPTIMIZE) -fno-exceptions $(PROFILE)
CFLAGS		:=$(OPTIMIZE) $(PROFILE)

include $(topdir)/build/warnings.mk

####### linker configuration

LDFLAGS = $(TARGET_LDFLAGS) $(PROFILE)
LDLIBS = $(TARGET_LDLIBS)

include $(topdir)/build/sdl.mk

.phony:

BUILDTESTS=\
	t/01_test_tap.exe

all: $(BUILDTESTS) RunWayPointParser-$(TARGET)$(TARGET_EXEEXT) \
	RunCanvas-$(TARGET)$(TARGET_EXEEXT) \
	RunDialog-$(TARGET)$(TARGET_EXEEXT)

test: .phony $(BUILDTESTS)
	# prove
	perl tools/testall.pl t/*

# TODO generalise
t/01_test_tap.exe: src/01_test_tap.c
	gcc -o $@ $<

RUN_WAY_POINT_PARSER_OBJS = \
	$(SRC)/Waypointparser.o \
	$(SRC)/WayPointList.o \
	$(SRC)/Math/Earth.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/UtilsText.o \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a \
	$(SRC)/LocalPath.o \
	src/RunWayPointParser.o
RunWayPointParser-$(TARGET)$(TARGET_EXEEXT): $(RUN_WAY_POINT_PARSER_OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_CANVAS_OBJS = \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/ContainerWindow.o \
	$(SRC)/Screen/TopWindow.o \
	$(SRC)/Screen/ButtonWindow.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/BufferCanvas.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Util.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/Screen/shapelib/mapsearch.o \
	$(SRC)/wcecompat/ts_string.o \
	src/RunCanvas.o
ifneq ($(ENABLE_SDL),y)
RUN_CANVAS_OBJS += $(SRC)/Screen/PaintCanvas.o
endif
RunCanvas-$(TARGET)$(TARGET_EXEEXT): $(RUN_CANVAS_OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

RUN_DIALOG_OBJS = \
	$(SRC)/xmlParser.o \
	$(SRC)/Dialogs/XML.o \
	$(SRC)/Dialogs/dlgComboPicker.o \
	$(SRC)/Math/FastMath.o \
	$(SRC)/Math/Geometry.o \
	$(SRC)/Screen/Animation.o \
	$(SRC)/Screen/Bitmap.o \
	$(SRC)/Screen/Brush.o \
	$(SRC)/Screen/Canvas.o \
	$(SRC)/Screen/VirtualCanvas.o \
	$(SRC)/Screen/BitmapCanvas.o \
	$(SRC)/Screen/Font.o \
	$(SRC)/Screen/Pen.o \
	$(SRC)/Screen/Window.o \
	$(SRC)/Screen/BufferWindow.o \
	$(SRC)/Screen/PaintCanvas.o \
	$(SRC)/Screen/PaintWindow.o \
	$(SRC)/Screen/ContainerWindow.o \
	$(SRC)/Screen/TextWindow.o \
	$(SRC)/Screen/EditWindow.o \
	$(SRC)/Screen/TopWindow.o \
	$(SRC)/Screen/Util.o \
	$(SRC)/Screen/shapelib/mapsearch.o \
	$(SRC)/DataField/Base.o \
	$(SRC)/DataField/Boolean.o \
	$(SRC)/DataField/ComboList.o \
	$(SRC)/DataField/Enum.o \
	$(SRC)/DataField/FileReader.o \
	$(SRC)/DataField/Float.o \
	$(SRC)/DataField/Integer.o \
	$(SRC)/DataField/String.o \
	$(SRC)/WindowControls.o \
	$(SRC)/UtilsText.o \
	$(SRC)/Dialogs/dlgHelp.o \
	$(SRC)/zzip-$(TARGET).a \
	$(SRC)/compat-$(TARGET).a \
	$(SRC)/wcecompat/ts_string.o \
	src/RunDialog.o
RunDialog-$(TARGET)$(TARGET_EXEEXT): $(RUN_DIALOG_OBJS:.o=-$(TARGET).o)
	@$(NQ)echo "  LINK    $@"
	$(Q)$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

include $(topdir)/build/zzip.mk
include $(topdir)/build/compat.mk

clean:
	rm -f $(BUILDTESTS) \
		RunWayPointParser-$(TARGET)$(TARGET_EXEEXT) \
		RunCanvas-$(TARGET)$(TARGET_EXEEXT) \
		RunDialog-$(TARGET)$(TARGET_EXEEXT)
	rm -f $(RUN_WAY_POINT_PARSER_OBJS:.o=-$(TARGET).o)
	rm -f $(RUN_DIALOG_OBJS:.o=-$(TARGET).o)
