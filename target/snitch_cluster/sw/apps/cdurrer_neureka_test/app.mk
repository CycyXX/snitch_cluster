APP              := cdurrer_neureka_test
$(APP)_BUILD_DIR := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/build
SRCS 			 := $(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/src/*.c)
# 					$(wildcard $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gen/src/*.c) \
# 					$(wildcard $(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/src/*.c)
$(APP)_INCDIRS   := $(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/inc \
					$(SN_ROOT)/target/snitch_cluster/sw/apps/$(APP)/gen/inc \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/inc \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/neureka/hal \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/neureka/gvsoc \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/neureka/bsp/ \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/neureka/bsp/testbench \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/neureka/bsp/siracusa \
					$(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/model/deps/pulp-nnx/util

include $(SN_ROOT)/target/snitch_cluster/sw/apps/common.mk
include $(SN_ROOT)/.bender/git/checkouts/neureka-e5b35a1ad071cd77/sw/sw.mk
