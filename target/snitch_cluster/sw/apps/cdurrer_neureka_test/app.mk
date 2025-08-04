APP              := cdurrer_neureka_test
$(APP)_BUILD_DIR := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/build
SRCS 			 := $(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/src/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gen/src/*.c)
$(APP)_INCDIRS   := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/data \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/inc \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gen/inc

include $(SN_ROOT)/target/snitch_cluster/sw/apps/common.mk
