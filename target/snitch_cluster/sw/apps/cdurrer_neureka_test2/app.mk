APP              := cdurrer_neureka_test2
$(APP)_BUILD_DIR := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/build
SRCS 			 := $(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/src/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/util/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/hal/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gvsoc/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/bsp/testbench/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/bsp/siracusa/*.c) \
					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gen/src/*.c)
$(APP)_INCDIRS   := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP) \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/inc \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/util \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/hal \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gvsoc \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/bsp \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/bsp/testbench \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/bsp/siracusa \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gen/inc


include $(SN_ROOT)/target/snitch_cluster/sw/apps/common.mk
