
DEPFILE		=$(dir $@).$(notdir $@).d
DEPFLAGS	=-Wp,-MM,-MF,$(DEPFILE)

.PHONY: FORCE

ifneq ($(wildcard $(SRC)/.*.d),)
#include $(wildcard $(SRC)/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/.*.d),)
#include $(wildcard $(SRC)/*/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/*/.*.d),)
#include $(wildcard $(SRC)/*/*/.*.d)
endif
ifneq ($(wildcard $(SRC)/*/*/*/.*.d),)
#include $(wildcard $(SRC)/*/*/*/.*.d)
endif
