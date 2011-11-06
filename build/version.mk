VERSION = $(strip $(shell cat $(topdir)/VERSION.txt))
FULL_VERSION = $(VERSION)

CPPFLAGS += -DXCSOAR_VERSION=\"$(VERSION)\"

GIT_COMMIT_ID := $(shell git rev-parse --short --verify HEAD 2>/dev/null)
RELEASE_COMMIT_ID := $(shell git rev-parse --short --verify "v$(VERSION)" 2>/dev/null)
# only append the commit id for unreleased builds (no release tag)
ifneq ($(GIT_COMMIT_ID),$(RELEASE_COMMIT_ID))
CPPFLAGS += -DGIT_COMMIT_ID=\"$(GIT_COMMIT_ID)\"
FULL_VERSION := $(FULL_VERSION)~$(GIT_COMMIT_ID)
endif

$(TARGET_OUTPUT_DIR)/src/Version.o: VERSION.txt
