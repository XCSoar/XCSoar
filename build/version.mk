CONFIG = $(topdir)/$(PROGRAM_NAME).config
include $(CONFIG)

# w/o VERSION.txt:
ifeq ($(PROGRAM_VERSION),"")
PROGRAM_VERSION = $(strip $(shell cat $(topdir)/VERSION.txt))
endif

### echo "$(PROGRAM_VERSION)"

FULL_VERSION = $(PROGRAM_VERSION)

VERSION_CPPFLAGS = -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\"

GIT_COMMIT_ID := $(shell git rev-parse --short --verify HEAD 2>$(NUL))
RELEASE_COMMIT_ID := $(shell git rev-parse --short --verify "v$(PROGRAM_VERSION)^{commit}" 2>$(NUL))
# only append the commit id for unreleased builds (no release tag)
ifneq ($(GIT_COMMIT_ID),$(RELEASE_COMMIT_ID))
VERSION_CPPFLAGS += -DGIT_COMMIT_ID=\"$(GIT_COMMIT_ID)\"
FULL_VERSION := $(FULL_VERSION)~$(GIT_COMMIT_ID)
endif

$(call SRC_TO_OBJ,$(SRC)/Version.cpp): $(topdir)/VERSION.txt
$(call SRC_TO_OBJ,$(SRC)/Version.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)
$(call SRC_TO_OBJ,$(SRC)/Dialogs/dlgCredits.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)

# August2111-Start
# ======================================================================
# CONFIG = $(topdir)/$(PROGRAM_NAME).config
# include $(CONFIG)
$(topdir)/output/include/ProgramVersion.h: $(topdir)/$(PROGRAM_NAME).config
	@$(NQ)echo "  VERSION:   $< == $@"
	$(Q)python3 $(topdir)/tools/python/replace.py  $(topdir)/$(PROGRAM_NAME).config $< $@ $(OUT)/include/ProgramVersion.h

# Version.o need the new PROGRAM_VERSION if it is available:
$(ABI_OUTPUT_DIR)/src/Version.o: $(topdir)/src/Version.cpp $(topdir)/$(PROGRAM_NAME).config $(topdir)/output/include/ProgramVersion.h
	@$(NQ)echo "  CPP     $@"
	$(Q)$(WRAPPED_CXX) $< -c -o $@ $(cxx-flags)
# ======================================================================
# August2111-End

