CONFIG = $(topdir)/OpenSoar.config
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
