# Rules for XCSoarLaunch.dll, the launcher for the PocketPC shell
SVG_LAUNCH = Data/graphics/launcher.svg
PNG_LAUNCH_224 = $(patsubst Data/graphics/%.svg,output/data/graphics/%-224.png,$(SVG_LAUNCH))
BMP_LAUNCH_FLY_224 = $(PNG_LAUNCH_224:.png=-1.bmp)
BMP_LAUNCH_SIM_224 = $(PNG_LAUNCH_224:.png=-2.bmp)

ifeq ($(HAVE_CE)$(findstring $(TARGET),ALTAIR ALTAIRPORTRAIT),y)


# render from SVG to PNG
$(PNG_LAUNCH_224): output/data/graphics/%-224.png: Data/graphics/%.svg | output/data/graphics/dirstamp
	@$(NQ)echo "  SVG     $@"
	$(Q)rsvg-convert --width=224 $< -o $@

# split into two uncompressed 8-bit BMPs (single 'convert' operation)
$(BMP_LAUNCH_FLY_224): %-1.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	@$(NQ)echo "  BMP     $(@:1.bmp=2.bmp)"
	$(Q)$(IM_PREFIX)convert $< -background blue -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 $(@:1.bmp=%d.bmp)
$(BMP_LAUNCH_SIM_224): %-2.bmp: %.png
	@$(NQ)echo "  BMP     $@"
	@$(NQ)echo "  BMP     $(@:1.bmp=2.bmp)"
	$(Q)$(IM_PREFIX)convert $< -background blue -layers flatten +matte +dither -compress none -type optimize -colors 256 -crop '50%x100%' -scene 1 $(@:1.bmp=%d.bmp)

LAUNCH_RESOURCE_FILES = $(BMP_LAUNCH_FLY_224) $(BMP_LAUNCH_SIM_224) 

XCSOARLAUNCH_DLL = $(TARGET_BIN_DIR)/XCSoarLaunch.dll
XCSOARLAUNCH_SOURCES = \
	$(SRC)/XCSoarLaunch.c
XCSOARLAUNCH_OBJS = $(call SRC_TO_OBJ,$(XCSOARLAUNCH_SOURCES))
$(XCSOARLAUNCH_OBJS): CFLAGS += -Wno-missing-declarations -Wno-missing-prototypes

$(TARGET_OUTPUT_DIR)/XCSoarLaunch.e: $(SRC)/XCSoarLaunch.def $(XCSOARLAUNCH_OBJS) | $(TARGET_BIN_DIR)/dirstamp
	$(Q)$(DLLTOOL) -e $@ -d $^

$(TARGET_OUTPUT_DIR)/XCSoarLaunch.rsc: Data/XCSoarLaunch.rc $(LAUNCH_RESOURCE_FILES) | $(TARGET_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  WINDRES $@"
	$(Q)$(WINDRES) $(WINDRESFLAGS) -o $@ $<

$(XCSOARLAUNCH_DLL): TARGET_LDLIBS = -laygshell
$(XCSOARLAUNCH_DLL): $(TARGET_OUTPUT_DIR)/XCSoarLaunch.e $(XCSOARLAUNCH_OBJS) $(TARGET_OUTPUT_DIR)/XCSoarLaunch.rsc | $(TARGET_BIN_DIR)/dirstamp
	$(CC) -shared $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

else

XCSOARLAUNCH_DLL =

endif
