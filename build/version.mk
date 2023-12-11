CONFIG = $(topdir)/OpenSoar.config
include $(CONFIG)

# w/o VERSION.txt:
ifeq ($(PROGRAM_VERSION),"")
    # take the version from XCSoar VERSION.txt
    PROGRAM_VERSION = $(strip $(shell cat $(topdir)/VERSION.txt))
endif

$(info PROGRAM_NAME      is $(PROGRAM_NAME))
$(info PROGRAM_VERSION   is $(PROGRAM_VERSION))

FULL_VERSION = $(PROGRAM_VERSION)

VERSION_CPPFLAGS = -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\"

GIT_COMMIT_ID := $(shell git rev-parse --short --verify HEAD )
GIT_TAG=tags/opensoar-$(PROGRAM_VERSION)
$(info GIT_TAG           is $(GIT_TAG) )
RELEASE_COMMIT_ID := $(shell git rev-parse --short --verify $(GIT_TAG)^0 )

$(info GIT_COMMIT_ID     is $(GIT_COMMIT_ID) )
$(info RELEASE_COMMIT_ID is $(RELEASE_COMMIT_ID) )

# only append the commit id for unreleased builds (no release tag)
ifneq ($(GIT_COMMIT_ID),$(RELEASE_COMMIT_ID) )
    $(info Git commits: HEAD = $(GIT_COMMIT_ID) vs. RELEASE = $(RELEASE_COMMIT_ID) )
    VERSION_CPPFLAGS += -DGIT_COMMIT_ID=\"$(GIT_COMMIT_ID)\"
    FULL_VERSION := $(FULL_VERSION)~$(GIT_COMMIT_ID)
endif

$(call SRC_TO_OBJ,$(SRC)/Version.cpp): $(topdir)/VERSION.txt
$(call SRC_TO_OBJ,$(SRC)/Version.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)
$(call SRC_TO_OBJ,$(SRC)/Dialogs/dlgCredits.cpp): CPPFLAGS += $(VERSION_CPPFLAGS)
