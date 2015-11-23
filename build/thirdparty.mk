ifeq ($(MAKECMDGOALS),kobo-libs) # kludge to allow bootstrapping kobo-libs
$(error Target "kobo-libs" is obsolete, please use "libs" instead)
endif

.PHONY: libs
libs:
	./build/thirdparty.py $(TARGET_OUTPUT_DIR) $(TARGET) $(HOST_TRIPLET) "$(TARGET_ARCH)" $(CC) $(CXX) $(AR) $(STRIP)
