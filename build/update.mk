# Update repository settings may be overridden by local-config.mk or the
# command line for rebranded builds.
UPDATE_REPOSITORY_TARGET ?= $(TARGET_FLAVOR)
UPDATE_ALLOWED_HOSTS ?= xcsoar.org,download.xcsoar.org,apps.apple.com

UPDATE_CPPFLAGS = -DUPDATE_REPOSITORY_TARGET=\"$(UPDATE_REPOSITORY_TARGET)\" \
	-DUPDATE_ALLOWED_HOSTS=\"$(UPDATE_ALLOWED_HOSTS)\"

$(call SRC_TO_OBJ,$(SRC)/Update/RepositoryBackend.cpp): CPPFLAGS += $(UPDATE_CPPFLAGS)
