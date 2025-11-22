# This Makefile fragment builds the Android package (XCSoar.apk) or bundle (XCSoar.aab).
# We're not using NDK's Makefiles because our Makefile is so big and
# complex, we don't want to duplicate that for another platform.

ifeq ($(TARGET),ANDROID)

### Toolchain paths

ifeq ($(HOST_IS_DARWIN),y)
  ANDROID_SDK ?= $(HOME)/Library/Android/sdk
else
  ANDROID_SDK ?= $(HOME)/opt/android-sdk-linux
endif
ANDROID_SDK_PLATFORM_DIR = $(ANDROID_SDK)/platforms/$(ANDROID_SDK_PLATFORM)

ANDROID_BUILD_TOOLS_DIR = $(ANDROID_SDK)/build-tools/35.0.0
APKSIGNER = $(ANDROID_BUILD_TOOLS_DIR)/apksigner
ZIPALIGN = $(ANDROID_BUILD_TOOLS_DIR)/zipalign
AAPT = $(ANDROID_BUILD_TOOLS_DIR)/aapt
AAPT2 = $(ANDROID_BUILD_TOOLS_DIR)/aapt2
D8 = $(ANDROID_BUILD_TOOLS_DIR)/d8
BUNDLETOOL = $(HOME)/opt/bundletool/bin/bundletool

### Directory structure

ifeq ($(ANDROID_BUNDLE_BUILD),y)
# Bundle build uses noarch directory
NO_ARCH_OUTPUT_DIR = $(TARGET_OUTPUT_DIR)/noarch
ANDROID_OUTPUT_DIR = $(NO_ARCH_OUTPUT_DIR)
RES_DIR = $(NO_ARCH_OUTPUT_DIR)/res
DRAWABLE_DIR = $(RES_DIR)/drawable
RAW_DIR = $(RES_DIR)/raw
COMPILED_RES_DIR = $(NO_ARCH_OUTPUT_DIR)/compiled_resources
GEN_DIR = $(NO_ARCH_OUTPUT_DIR)/gen
PROTOBUF_OUT_DIR = $(NO_ARCH_OUTPUT_DIR)/proto_out
NATIVE_INCLUDE = $(TARGET_OUTPUT_DIR)/include
NATIVE_INCLUDE_DIR = $(NATIVE_INCLUDE)
BUNDLE_BUILD_DIR = $(TARGET_OUTPUT_DIR)/$(XCSOAR_ABI)/build
ANDROID_BUNDLE_BASE = $(BUNDLE_BUILD_DIR)/base_module
ANDROID_ABI_DIR = $(ANDROID_BUNDLE_BASE)/lib/$(ANDROID_APK_LIB_ABI)
ANDROID_BUILD = $(BUNDLE_BUILD_DIR)
else
# APK build uses android directory
ANDROID_OUTPUT_DIR = $(TARGET_OUTPUT_DIR)/android
RES_DIR = $(ANDROID_OUTPUT_DIR)/res
DRAWABLE_DIR = $(RES_DIR)/drawable
RAW_DIR = $(RES_DIR)/raw
GEN_DIR = $(ANDROID_OUTPUT_DIR)/gen
NATIVE_INCLUDE = $(TARGET_OUTPUT_DIR)/include
NATIVE_INCLUDE_DIR = $(NATIVE_INCLUDE)
ANDROID_BUILD = $(TARGET_OUTPUT_DIR)/$(XCSOAR_ABI)/build
ANDROID_ABI_DIR = $(ANDROID_BUILD)/lib/$(ANDROID_APK_LIB_ABI)
endif

ANDROID_BIN = $(TARGET_BIN_DIR)

ANDROID_LIB_NAMES = xcsoar

APKSIGN = $(APKSIGNER) sign
ifeq ($(V),2)
APKSIGN += --verbose
endif

APKSIGN_RELEASE = $(APKSIGN)

# The environment variable ANDROID_KEYSTORE_PASS may be used to
# specify the keystore password; if you don't set it, you will be
# asked interactively
ifeq ($(origin ANDROID_KEYSTORE_PASS),environment)
APKSIGN_RELEASE += --ks-pass env:ANDROID_KEYSTORE_PASS
JARSIGNER_RELEASE_PASSWD = -storepass:env ANDROID_KEYSTORE_PASS
BUNDLETOOL_RELEASE_PASSWD = "--ks-pass=pass:$(ANDROID_KEYSTORE_PASS)"
endif

# Allow override of package name for different release channels
# Default to org.xcsoar (or org.xcsoar.testing if TESTING=y), but can be overridden:
#   make JAVA_PACKAGE=org.xcsoar.play ...
#   make JAVA_PACKAGE=org.xcsoar.foss ...
ifeq ($(TESTING),y)
JAVA_PACKAGE ?= org.xcsoar.testing
else
JAVA_PACKAGE ?= org.xcsoar
endif

NATIVE_CLASSES := \
	FileProvider \
	TextEntryDialog \
	NativeView \
	EventBridge \
	NativeSensorListener \
	NativeInputListener \
	DownloadUtil \
	BatteryReceiver \
	NativePortListener \
	NativeDetectDeviceListener
NATIVE_SOURCES = $(patsubst %,android/src/%.java,$(NATIVE_CLASSES))
NATIVE_PREFIX = $(NATIVE_INCLUDE)/$(subst .,_,$(JAVA_PACKAGE))_
NATIVE_HEADERS = $(patsubst %,$(NATIVE_PREFIX)%.h,$(NATIVE_CLASSES))

JAVA_SOURCES := \
	$(wildcard android/src/*.java) \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/CH34xIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/CP210xIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/CP2130Ids.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/FTDISioIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/Helpers.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/PL2303Ids.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/deviceids/XdcVcpIds.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/AbstractWorkerThread.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/Buffer.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CDCSerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CH34xSerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CP2102SerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/CP2130SpiDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/PL2303SerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/FTDISerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/SerialBuffer.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/SerialInputStream.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/SerialOutputStream.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSerialDebugger.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSerialDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSerialInterface.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSpiDevice.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/usbserial/UsbSpiInterface.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/utils/HexData.java \
	android/UsbSerial/usbserial/src/main/java/com/felhr/utils/SafeUsbRequest.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/AnalogInput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/CapSense.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/Closeable.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/DigitalInput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/DigitalOutput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IcspMaster.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IOIOConnection.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IOIO.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/IOIOFactory.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/PulseInput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/PwmOutput.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/Sequencer.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/SpiMaster.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/TwiMaster.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/Uart.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/exception/ConnectionLostException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/exception/IncompatibilityException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/api/exception/OutOfResourceException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/AbstractPin.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/AbstractResource.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/AnalogInputImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/Board.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/CapSenseImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/Constants.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/DigitalInputImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/DigitalOutputImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/FixedReadBufferedInputStream.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/FlowControlledOutputStream.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/FlowControlledPacketSender.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/GenericResourceAllocator.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IcspMasterImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IncapImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IncomingState.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/InterruptibleQueue.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IOIOImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/IOIOProtocol.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/PwmImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/QueueInputStream.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/ResourceLifeCycle.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/ResourceManager.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SequencerImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SocketIOIOConnectionBootstrap.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SocketIOIOConnection.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SpecificResourceAllocator.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/SpiMasterImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/TwiMasterImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/UartImpl.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/impl/Version.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/IOIOConnectionBootstrap.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/IOIOConnectionFactory.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/Log.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/spi/NoRuntimeSupportException.java \
	android/ioio/IOIOLibCore/src/main/java/ioio/lib/util/IOIOConnectionRegistry.java \
	android/ioio/IOIOLibAndroid/src/main/java/ioio/lib/spi/LogImpl.java \
	android/ioio/IOIOLibAndroid/src/main/java/ioio/lib/util/android/ContextWrapperDependent.java \
	android/ioio/IOIOLibAndroidAccessory/src/main/java/ioio/lib/android/accessory/Adapter.java \
	android/ioio/IOIOLibAndroidAccessory/src/main/java/ioio/lib/android/accessory/AccessoryConnectionBootstrap.java \
	android/ioio/IOIOLibAndroidBluetooth/src/main/java/ioio/lib/android/bluetooth/BluetoothIOIOConnectionBootstrap.java \
	android/ioio/IOIOLibAndroidBluetooth/src/main/java/ioio/lib/android/bluetooth/BluetoothIOIOConnection.java \
	android/ioio/IOIOLibAndroidDevice/src/main/java/ioio/lib/android/device/DeviceConnectionBootstrap.java \
	android/ioio/IOIOLibAndroidDevice/src/main/java/ioio/lib/android/device/Streams.java

JAVA_CLASSFILES_DIR = $(ANDROID_OUTPUT_DIR)/classes

ifeq ($(TESTING),y)
ICON_SVG = $(topdir)/Data/graphics/logo_red.svg
else
ICON_SVG = $(topdir)/Data/graphics/logo.svg
endif

ICON_WHITE_SVG = $(topdir)/Data/graphics/logo_white.svg

$(RES_DIR)/drawable-ldpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-ldpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(RES_DIR)/drawable/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(RES_DIR)/drawable-hdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(RES_DIR)/drawable-xhdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=96 $< -o $@

$(RES_DIR)/drawable-xxhdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-xxhdpi/dirstamp
	$(Q)rsvg-convert --width=144 $< -o $@

$(RES_DIR)/drawable-xxxhdpi/icon.png: $(ICON_SVG) | $(RES_DIR)/drawable-xxxhdpi/dirstamp
	$(Q)rsvg-convert --width=192 $< -o $@

$(RES_DIR)/drawable/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable/dirstamp
	$(Q)rsvg-convert --width=24 $< -o $@

$(RES_DIR)/drawable-hdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-hdpi/dirstamp
	$(Q)rsvg-convert --width=36 $< -o $@

$(RES_DIR)/drawable-xhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-xhdpi/dirstamp
	$(Q)rsvg-convert --width=48 $< -o $@

$(RES_DIR)/drawable-xxhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-xxhdpi/dirstamp
	$(Q)rsvg-convert --width=72 $< -o $@

$(RES_DIR)/drawable-xxxhdpi/notification_icon.png: $(ICON_WHITE_SVG) | $(RES_DIR)/drawable-xxxhdpi/dirstamp
	$(Q)rsvg-convert --width=96 $< -o $@

OGGENC = oggenc --quiet --quality 1

SOUNDS = fail insert remove beep_bweep beep_clear beep_drip
SOUND_FILES = $(patsubst %,$(RAW_DIR)/%.ogg,$(SOUNDS))

$(SOUND_FILES): $(RAW_DIR)/%.ogg: Data/sound/%.wav | $(RAW_DIR)/dirstamp
	@$(NQ)echo "  OGGENC  $@"
	$(Q)$(OGGENC) -o $@ $<

# IM_CONVERT is defined in build/imagemagick.mk

PNG1 := $(patsubst Data/bitmaps/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_BITMAPS))

$(PNG1): $(DRAWABLE_DIR)/%.png: Data/bitmaps/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_CONVERT) $< $@

PNG2 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_LAUNCH_ALL))
$(PNG2): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_CONVERT) $< $@

PNG3 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_SPLASH_80) $(BMP_SPLASH_160) $(BMP_TITLE_110) $(BMP_TITLE_320))
$(PNG3): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_CONVERT) $< $@

PNG4 := $(patsubst $(DATA)/icons/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_ICONS_ALL))
$(PNG4): $(DRAWABLE_DIR)/%.png: $(DATA)/icons/%.png | $(DRAWABLE_DIR)/dirstamp
	$(Q)cp $< $@

PNG5 := $(patsubst $(DATA)/graphics/%.bmp,$(DRAWABLE_DIR)/%.png,$(BMP_DIALOG_TITLE) $(BMP_PROGRESS_BORDER))
$(PNG5): $(DRAWABLE_DIR)/%.png: $(DATA)/graphics/%.bmp | $(DRAWABLE_DIR)/dirstamp
	$(Q)$(IM_CONVERT) $< $@

PNG_FILES = $(PNG1) $(PNG1b) $(PNG2) $(PNG3) $(PNG4) $(PNG5) \
	$(RES_DIR)/drawable-ldpi/icon.png \
	$(RES_DIR)/drawable/icon.png \
	$(RES_DIR)/drawable-hdpi/icon.png \
	$(RES_DIR)/drawable-xhdpi/icon.png \
	$(RES_DIR)/drawable-xxhdpi/icon.png \
	$(RES_DIR)/drawable-xxxhdpi/icon.png \
	$(RES_DIR)/drawable/notification_icon.png \
	$(RES_DIR)/drawable-hdpi/notification_icon.png \
	$(RES_DIR)/drawable-xhdpi/notification_icon.png \
	$(RES_DIR)/drawable-xxhdpi/notification_icon.png \
	$(RES_DIR)/drawable-xxxhdpi/notification_icon.png

# Use template manifest for all builds
MANIFEST_TEMPLATE = android/AndroidManifest.xml.template

# Determine package name for manifest based on build flags
# Priority: FOSS > PLAY > TESTING > default
ifeq ($(FOSS),y)
MANIFEST_PACKAGE = org.xcsoar.foss
MANIFEST_APP_LABEL = @string/app_name
else ifeq ($(PLAY),y)
MANIFEST_PACKAGE = org.xcsoar.play
MANIFEST_APP_LABEL = @string/app_name
else ifeq ($(TESTING),y)
MANIFEST_PACKAGE = org.xcsoar.testing
MANIFEST_APP_LABEL = @string/app_name_testing
else
MANIFEST_PACKAGE = org.xcsoar
MANIFEST_APP_LABEL = @string/app_name
endif

# Generate a processed manifest with the custom package name
ifeq ($(ANDROID_BUNDLE_BUILD),y)
MANIFEST_PROCESSED = $(NO_ARCH_OUTPUT_DIR)/AndroidManifest.xml
MANIFEST_PACKAGE_STAMP = $(NO_ARCH_OUTPUT_DIR)/.manifest_package.stamp
else
MANIFEST_PROCESSED = $(ANDROID_OUTPUT_DIR)/AndroidManifest.xml
MANIFEST_PACKAGE_STAMP = $(ANDROID_OUTPUT_DIR)/.manifest_package.stamp
endif
R_JAVA_PATH = $(GEN_DIR)/$(subst .,/,$(JAVA_PACKAGE))/R.java

# Stamp file that tracks the current package name to ensure rebuild when flags change
$(MANIFEST_PACKAGE_STAMP): FORCE | $(ANDROID_OUTPUT_DIR)/dirstamp
	@if [ ! -f $@ ] || [ "$$(cat $@ 2>/dev/null)" != "$(MANIFEST_PACKAGE)" ]; then \
		echo "$(MANIFEST_PACKAGE)" > $@.tmp && mv $@.tmp $@; \
	fi

$(MANIFEST_PROCESSED): $(MANIFEST_TEMPLATE) $(MANIFEST_PACKAGE_STAMP) | $(ANDROID_OUTPUT_DIR)/dirstamp
	@$(NQ)echo "  PROCESS $@"
	$(Q)sed -e 's/@PACKAGE_NAME@/$(MANIFEST_PACKAGE)/g' \
		-e 's|@APP_LABEL@|$(MANIFEST_APP_LABEL)|g' \
		$< > $@
	$(Q)echo "$(MANIFEST_PACKAGE)" > $(MANIFEST_PACKAGE_STAMP)

ANDROID_XML_RES := $(wildcard android/res/*/*.xml)
ANDROID_XML_RES_COPIES := $(patsubst android/res/%,$(RES_DIR)/%,$(ANDROID_XML_RES))

$(ANDROID_XML_RES_COPIES): $(RES_DIR)/%: android/res/%
	$(Q)-$(MKDIR) -p $(dir $@)
	$(Q)cp $< $@

### Resource compilation

ifeq ($(ANDROID_BUNDLE_BUILD),y)
# Bundle build: Use AAPT2 with protobuf format
$(PROTOBUF_OUT_DIR)/dirstamp: $(PNG_FILES) $(SOUND_FILES) $(ANDROID_XML_RES_COPIES) $(MANIFEST_PROCESSED) | $(GEN_DIR)/dirstamp $(COMPILED_RES_DIR)/dirstamp
	@$(NQ)echo "  AAPT2"
	$(Q)$(AAPT2) compile \
		-o $(COMPILED_RES_DIR) \
		--dir $(RES_DIR)
	$(Q)rm -f $(COMPILED_RES_DIR)/*dirstamp.flat
	$(Q)$(AAPT2) link --proto-format --auto-add-overlay \
		--custom-package $(JAVA_PACKAGE) \
		--manifest $(MANIFEST_PROCESSED) \
		-R $(COMPILED_RES_DIR)/*.flat \
		--java $(GEN_DIR) \
		-I $(ANDROID_SDK_PLATFORM_DIR)/android.jar \
		-o $(NO_ARCH_OUTPUT_DIR)/resources.apk
	$(Q)$(UNZIP) -o $(NO_ARCH_OUTPUT_DIR)/resources.apk \
		-d $(PROTOBUF_OUT_DIR)
	$(Q)touch $@

# R.java is generated by aapt2, when base package is generated
$(R_JAVA_PATH): $(PROTOBUF_OUT_DIR)/dirstamp
	@$(NQ)echo "  MKDIR   $(dir $@)"
	$(Q)mkdir -p $(dir $@)
else
# APK build: Use AAPT
$(ANDROID_OUTPUT_DIR)/resources.apk: $(PNG_FILES) $(SOUND_FILES) $(ANDROID_XML_RES_COPIES) $(MANIFEST_PROCESSED) | $(GEN_DIR)/dirstamp
	@$(NQ)echo "  AAPT"
	$(Q)$(AAPT) package -f -m --auto-add-overlay \
		--custom-package $(JAVA_PACKAGE) \
		-M $(MANIFEST_PROCESSED) \
		-S $(RES_DIR) \
		-J $(GEN_DIR) \
		-I $(ANDROID_SDK_PLATFORM_DIR)/android.jar \
		-F $(ANDROID_OUTPUT_DIR)/resources.apk

# R.java is generated by aapt, when resources.apk is generated
# Ensure the directory exists before aapt runs
$(R_JAVA_PATH): $(ANDROID_OUTPUT_DIR)/resources.apk
	@$(NQ)echo "  MKDIR   $(dir $@)"
	$(Q)mkdir -p $(dir $@)
endif

### Java compilation

ifeq ($(ANDROID_BUNDLE_BUILD),y)
# Bundle build: Output to classes.zip
$(NO_ARCH_OUTPUT_DIR)/classes.zip: $(JAVA_SOURCES) $(R_JAVA_PATH) | $(JAVA_CLASSFILES_DIR)/dirstamp
	@$(NQ)echo "  JAVAC   $(JAVA_CLASSFILES_DIR)"
	$(Q)$(JAVAC) \
		-source 1.8 -target 1.8 \
		-Xlint:all \
		-Xlint:-deprecation \
		-Xlint:-options \
		-Xlint:-static \
		-Xlint:-this-escape \
		-Xlint:-removal \
		-cp $(ANDROID_SDK_PLATFORM_DIR)/android.jar:$(JAVA_CLASSFILES_DIR) \
		-d $(JAVA_CLASSFILES_DIR) $(R_JAVA_PATH) \
		-h $(NATIVE_INCLUDE_DIR) \
		$(JAVA_SOURCES)
	$(Q)$(ZIP) -0 -r $(NO_ARCH_OUTPUT_DIR)/classes.zip $(JAVA_CLASSFILES_DIR)
else
# APK build: Output to classes.jar
$(ANDROID_OUTPUT_DIR)/classes.jar: $(JAVA_SOURCES) $(R_JAVA_PATH) | $(JAVA_CLASSFILES_DIR)/dirstamp
	@$(NQ)echo "  JAVAC   $(JAVA_CLASSFILES_DIR)"
	$(Q)$(filter-out -Werror,$(JAVAC)) \
		-source 1.8 -target 1.8 \
		-Xlint:all \
		-Xlint:-deprecation \
		-Xlint:-options \
		-Xlint:-static \
		-Xlint:-this-escape \
		-Xlint:-removal \
		-Xlint:-processing \
		-g:source,lines,vars \
		-cp $(ANDROID_SDK_PLATFORM_DIR)/android.jar:$(JAVA_CLASSFILES_DIR) \
		-d $(JAVA_CLASSFILES_DIR) $(R_JAVA_PATH) \
		-h $(NATIVE_INCLUDE) \
		$(JAVA_SOURCES)
	$(Q)cd $(JAVA_CLASSFILES_DIR) && $(ZIP) -0 -r $(abspath $(ANDROID_OUTPUT_DIR)/classes.jar) .
endif

### DEX conversion

ifeq ($(ANDROID_BUNDLE_BUILD),y)
CLASSES_INPUT = $(NO_ARCH_OUTPUT_DIR)/classes.zip
CLASSES_DEX = $(NO_ARCH_OUTPUT_DIR)/classes.dex
else
CLASSES_INPUT = $(ANDROID_OUTPUT_DIR)/classes.jar
CLASSES_DEX = $(ANDROID_OUTPUT_DIR)/classes.dex
endif

# Note: desugaring causes crashes on Android 13 (Pixel 6); as a
# workaround, it's disabled for now.
$(CLASSES_DEX): $(CLASSES_INPUT)
	@$(NQ)echo "  D8      $@"
	$(Q)$(D8) \
		--no-desugaring \
		--min-api 21 \
		--lib $(ANDROID_SDK_PLATFORM_DIR)/android.jar \
		--output $(dir $@) $(CLASSES_INPUT)

# Native headers generated at Java compile step
$(NATIVE_HEADERS): $(CLASSES_DEX)

### Native libraries build

ifeq ($(FAT_BINARY),y)

# generate a "fat" APK/AAB file with binaries for all ABIs

ANDROID_LIB_BUILD =
ANDROID_THIRDPARTY_STAMPS =

# Example: $(eval $(call generate-abi,xcsoar,armeabi-v7a,ANDROID7))
define generate-abi

ifeq ($(ANDROID_BUNDLE_BUILD),y)
ANDROID_LIB_BUILD += $$(ANDROID_BUNDLE_BASE)/lib/$(2)/lib$(1).so
else
ANDROID_LIB_BUILD += $$(ANDROID_BUILD)/lib/$(2)/lib$(1).so
endif

ifeq ($(ANDROID_BUNDLE_BUILD),y)
# copy libxcsoar.so to bundle
$$(ANDROID_BUNDLE_BASE)/lib/$(2)/lib$(1).so: $$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1).so | $$(ANDROID_BUNDLE_BASE)/lib/$(2)/dirstamp
	$$(Q)cp $$< $$@
else
# copy libxcsoar.so to APK
$$(ANDROID_BUILD)/lib/$(2)/lib$(1).so: $$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1).so | $$(ANDROID_BUILD)/lib/$(2)/dirstamp
	$$(Q)cp $$< $$@
endif

# build third-party libraries
ANDROID_THIRDPARTY_STAMPS += $$(TARGET_OUTPUT_DIR)/$(2)/thirdparty.stamp
$$(TARGET_OUTPUT_DIR)/$(2)/thirdparty.stamp: FORCE
	$$(Q)$$(MAKE) TARGET_OUTPUT_DIR=$$(TARGET_OUTPUT_DIR) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) libs

# build libxcsoar.so
$$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1).so: $(NATIVE_HEADERS) generate boost FORCE
	$$(Q)$$(MAKE) TARGET_OUTPUT_DIR=$$(TARGET_OUTPUT_DIR) TARGET=$(3) DEBUG=$$(DEBUG) USE_CCACHE=$$(USE_CCACHE) $$@

# extract symbolication files for Google Play
ifeq ($(ANDROID_BUNDLE_BUILD),y)
ANDROID_SYMBOLICATION_BUILD += $$(BUNDLE_BUILD_DIR)/symbols/$(2)/lib$(1).so
$$(BUNDLE_BUILD_DIR)/symbols/$(2)/lib$(1).so: $$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1)-ns.so | $$(BUNDLE_BUILD_DIR)/symbols/$(2)/dirstamp
	$$(Q)$$(TCPREFIX)objcopy$$(EXE) --strip-debug $$< $$@
else
ANDROID_SYMBOLICATION_BUILD += $$(ANDROID_BUILD)/symbols/$(2)/lib$(1).so
$$(ANDROID_BUILD)/symbols/$(2)/lib$(1).so: $$(TARGET_OUTPUT_DIR)/$(2)/$$(XCSOAR_ABI)/bin/lib$(1)-ns.so | $$(ANDROID_BUILD)/symbols/$(2)/dirstamp
	$$(Q)$$(TCPREFIX)objcopy$$(EXE) --strip-debug $$< $$@
endif

endef

# Example: $(eval $(call generate-abi,xcsoar))
define generate-all-abis
$(eval $(call generate-abi,$(1),armeabi-v7a,ANDROID7))
$(eval $(call generate-abi,$(1),x86,ANDROID86))
$(eval $(call generate-abi,$(1),arm64-v8a,ANDROIDAARCH64))
$(eval $(call generate-abi,$(1),x86_64,ANDROIDX64))
endef

$(foreach NAME,$(ANDROID_LIB_NAMES),$(eval $(call generate-all-abis,$(NAME))))

.PHONY: libs compile
libs: $(ANDROID_THIRDPARTY_STAMPS)
compile: $(ANDROID_LIB_BUILD)

# Generate symbols.zip (symbolication file) for Google Play, which
# allows Google Play to show symbol names in stack traces.
ifeq ($(ANDROID_BUNDLE_BUILD),y)
$(TARGET_OUTPUT_DIR)/symbols.zip: $(ANDROID_SYMBOLICATION_BUILD)
	cd $(BUNDLE_BUILD_DIR)/symbols && $(ZIP) $(abspath $@) */*.so
else
$(TARGET_OUTPUT_DIR)/symbols.zip: $(ANDROID_SYMBOLICATION_BUILD)
	cd $(ANDROID_BUILD)/symbols && $(ZIP) $(abspath $@) */*.so
endif

else # !FAT_BINARY

# add dependency to this source file
$(call SRC_TO_OBJ,$(SRC)/Android/Main.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/EventBridge.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeSensorListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeDetectDeviceListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/Battery.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativePortListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/NativeInputListener.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/DownloadManager.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/TextEntryDialog.cpp): $(NATIVE_HEADERS)
$(call SRC_TO_OBJ,$(SRC)/Android/FileProvider.cpp): $(NATIVE_HEADERS)

ANDROID_LIB_BUILD = $(patsubst %,$(ANDROID_ABI_DIR)/lib%.so,$(ANDROID_LIB_NAMES))
$(ANDROID_LIB_BUILD): $(ANDROID_ABI_DIR)/lib%.so: $(ABI_BIN_DIR)/lib%.so | $(ANDROID_ABI_DIR)/dirstamp
	$(Q)cp $< $@

endif # !FAT_BINARY

### Keystores

# Generate ~/.android/debug.keystore, if it does not exists, as the official
# Android build tools do it:
ifeq ($(ANDROID_BUNDLE_BUILD),y)
DEBUG_KEYSTORE = $(HOME)/.android/debug.keystore
DEBUG_KEY_ALIAS = androiddebugkey
DEBUG_KEY_PASSWORD = android
$(DEBUG_KEYSTORE):
	@$(NQ)echo "  KEYTOOL $@"
	$(Q)-$(MKDIR) -p $(dir $@)
	$(Q)$(KEYTOOL) -genkey -noprompt \
		-keystore $@ \
		-storepass $(DEBUG_KEY_PASSWORD) \
		-alias $(DEBUG_KEY_ALIAS) \
		-keypass $(DEBUG_KEY_PASSWORD) \
		-dname "CN=Android Debug" \
		-keyalg RSA -keysize 2048 -validity 10000
else
$(HOME)/.android/debug.keystore:
	@$(NQ)echo "  KEYTOOL $@"
	$(Q)-$(MKDIR) -p $(HOME)/.android
	$(Q)$(KEYTOOL) -genkey -noprompt \
		-keystore $@ \
		-storepass android \
		-alias androiddebugkey \
		-keypass android \
		-dname "CN=Android Debug" \
		-keyalg RSA -keysize 2048 -validity 10000
endif

# Release keystore
ANDROID_KEYSTORE ?= $(HOME)/.android/mk.keystore
ANDROID_KEY_ALIAS ?= mk

### Final package build

ifeq ($(ANDROID_BUNDLE_BUILD),y)

# Bundle build targets

$(BUNDLE_BUILD_DIR)/base.zip: $(PROTOBUF_OUT_DIR)/dirstamp $(NO_ARCH_OUTPUT_DIR)/classes.dex $(ANDROID_LIB_BUILD) | $(BUNDLE_BUILD_DIR)/dirstamp
	@$(NQ)echo "  ZIP     $(notdir $@)"
	$(Q)mkdir -p $(ANDROID_BUNDLE_BASE) && \
		cp -r $(PROTOBUF_OUT_DIR)/res $(PROTOBUF_OUT_DIR)/resources.pb $(ANDROID_BUNDLE_BASE)
	$(Q)mkdir -p $(ANDROID_BUNDLE_BASE)/manifest && \
		cp $(PROTOBUF_OUT_DIR)/AndroidManifest.xml $(ANDROID_BUNDLE_BASE)/manifest/
	$(Q)mkdir -p $(ANDROID_BUNDLE_BASE)/dex && \
		cp $(NO_ARCH_OUTPUT_DIR)/classes.dex $(ANDROID_BUNDLE_BASE)/dex/
	$(Q)cd $(ANDROID_BUNDLE_BASE) && $(ZIP) -r $(abspath $@) . --exclude "*/dirstamp"

$(BUNDLE_BUILD_DIR)/unsigned.aab: $(BUNDLE_BUILD_DIR)/base.zip
	@$(NQ)echo "  BUNDLE  $(notdir $@)"
	$(Q)$(BUNDLETOOL) build-bundle --overwrite --modules $< --output $@

# Debug targets
.DELETE_ON_ERROR: $(ANDROID_BIN)/XCSoar-debug.aab
$(ANDROID_BIN)/XCSoar-debug.aab: $(BUNDLE_BUILD_DIR)/unsigned.aab $(DEBUG_KEYSTORE) | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)cp $< $@
	$(Q)$(JARSIGNER) -keystore $(DEBUG_KEYSTORE) -storepass $(DEBUG_KEY_PASSWORD) $@ $(DEBUG_KEY_ALIAS)

$(ANDROID_BIN)/XCSoar-debug.apk: $(ANDROID_BIN)/XCSoar-debug.aab $(DEBUG_KEYSTORE)
	@$(NQ)echo "  APK     $@"
	$(Q)$(BUNDLETOOL) build-apks --overwrite --mode=universal \
		--ks=$(DEBUG_KEYSTORE) --ks-pass=pass:$(DEBUG_KEY_PASSWORD) --ks-key-alias=$(DEBUG_KEY_ALIAS) \
		--bundle=$< \
		--output=$(BUNDLE_BUILD_DIR)/apkset-debug.apks
	$(Q)$(UNZIP) -p $(BUNDLE_BUILD_DIR)/apkset-debug.apks universal.apk > $@

# Release targets (signed)
.DELETE_ON_ERROR: $(ANDROID_BIN)/XCSoar.aab
$(ANDROID_BIN)/XCSoar.aab: $(BUNDLE_BUILD_DIR)/unsigned.aab | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)cp $< $@
	$(Q)$(JARSIGNER) -keystore $(ANDROID_KEYSTORE) $(JARSIGNER_RELEASE_PASSWD) $@ $(ANDROID_KEY_ALIAS)

$(ANDROID_BIN)/XCSoar.apk: $(ANDROID_BIN)/XCSoar.aab
	@$(NQ)echo "  APK     $@"
	$(Q)$(BUNDLETOOL) build-apks --overwrite --mode=universal \
		--ks=$(ANDROID_KEYSTORE) --ks-key-alias=$(ANDROID_KEY_ALIAS) $(BUNDLETOOL_RELEASE_PASSWD) \
		--bundle=$< \
		--output=$(BUNDLE_BUILD_DIR)/apkset-release.apks
	$(Q)$(UNZIP) -p $(BUNDLE_BUILD_DIR)/apkset-release.apks universal.apk > $@

else

# APK build targets

.DELETE_ON_ERROR: $(ANDROID_BUILD)/unsigned.apk
$(ANDROID_BUILD)/unsigned.apk: $(ANDROID_OUTPUT_DIR)/classes.dex $(ANDROID_OUTPUT_DIR)/resources.apk $(ANDROID_LIB_BUILD)
	@$(NQ)echo "  APK     $@"
	$(Q)cp $(ANDROID_OUTPUT_DIR)/classes.dex $(dir $@)/
	$(Q)cp $(ANDROID_OUTPUT_DIR)/resources.apk $@
	$(Q)cd $(dir $@) && $(ZIP) -r $(notdir $@) classes.dex lib/*/*.so

.DELETE_ON_ERROR: $(ANDROID_BUILD)/aligned.apk
$(ANDROID_BUILD)/aligned.apk: $(ANDROID_BUILD)/unsigned.apk
	@$(NQ)echo "  ALIGN   $@"
	$(Q)$(ZIPALIGN) -f 8 $< $@

$(ANDROID_BIN)/XCSoar-debug.apk: $(ANDROID_BUILD)/aligned.apk $(HOME)/.android/debug.keystore | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(APKSIGN) --in $< --out $@ --debuggable-apk-permitted -ks $(HOME)/.android/debug.keystore --ks-key-alias androiddebugkey --ks-pass pass:android

$(ANDROID_BIN)/XCSoar.apk: $(ANDROID_BUILD)/aligned.apk | $(ANDROID_BIN)/dirstamp
	@$(NQ)echo "  SIGN    $@"
	$(Q)$(APKSIGN_RELEASE) --in $< --out $@ -ks $(ANDROID_KEYSTORE) --ks-key-alias $(ANDROID_KEY_ALIAS)

endif

endif
