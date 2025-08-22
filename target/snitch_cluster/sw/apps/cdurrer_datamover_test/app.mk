APP              := cdurrer_datamover_test
$(APP)_BUILD_DIR := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/build
SRCS             := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/data

include $(SN_ROOT)/target/snitch_cluster/sw/apps/common.mk
